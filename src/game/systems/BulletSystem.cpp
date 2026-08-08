#include "BulletSystem.h"

#include <cmath>
#include <vector>

#include <assets/AnimationClip.h>
#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/AnimationSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/systems/SceneGraphSystem.h>
#include <utility/Logger.h>

#include <glm/gtx/quaternion.hpp>

#include "../GameConfig.h"
#include "../components/BulletComponent.h"
#include "../components/HealthComponent.h"

using namespace vortex;
using namespace utility;

namespace
{
	constexpr float INACTIVE_OFFSET = 2000.0f;

	float rotation_angle_from_up( glm::vec2 direction )
	{
		return std::atan2( direction.x, direction.y );
	}
}

BulletSystem::BulletSystem( eage::ecs::ECSRegistry& registry, eage::ecs::PhysicsSystem& physics_system,
							eage::ecs::AnimationSystem& animation_system,
							eage::ecs::SceneGraphSystem& scene_graph_system )
	: mRegistry( registry )
	, mPhysicsSystem( physics_system )
	, mAnimationSystem( animation_system )
	, mSceneGraphSystem( scene_graph_system )
{
	mPhysicsSystem.Subscribe( this );

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) * 0.5f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) * 0.5f;
	mScreenBottomRight = glm::vec2( half_width, -half_height );
}

BulletSystem::~BulletSystem()
{
	mPhysicsSystem.Unsubscribe( this );
}

BulletPoolId
BulletSystem::PreparePool( eage::ecs::RenderSystem& render_system, const BulletPoolConfig& config, int count, uint64_t root_entity )
{
	BulletPoolId pool_id = mNextPoolId++;
	auto& pool = mPools[pool_id];
	mPoolFireInterval[pool_id] = config.fire_interval;

	glm::vec2 inactive_pos = mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, -INACTIVE_OFFSET );

	float mesh_width = config.mesh_width;
	float mesh_height = config.mesh_height;
	uint32_t texture_index = config.texture_index;

	if( config.clip_id != eage::ecs::INVALID_ID )
	{
		const assets::AnimationClip* clip = mAnimationSystem.GetClip( config.clip_id );
		if( clip != nullptr && clip->GetFrameCount() > 0 )
		{
			const glm::ivec2 frame_size = clip->GetFrameSize();
			if( frame_size.x > 0 && frame_size.y > 0 )
			{
				mesh_width = static_cast<float>( frame_size.x );
				mesh_height = static_cast<float>( frame_size.y );
			}
			texture_index = clip->GetFrameTexture( 0 );
		}
	}

	eage::ecs::ResourceId mesh_id = render_system.CreateSpriteMesh( mesh_width, mesh_height );

	for( int i = 0; i < count; ++i )
	{
		auto entity = mRegistry.CreateEntity();
		pool.push_back( entity );
		mEntityToPool[entity] = pool_id;

		mSceneGraphSystem.AddNodeToParent( entity, root_entity );

		eage::ecs::TransformComponent transform;
		transform.SetPosition( glm::vec3( inactive_pos, 0.f ) );
		mRegistry.AddComponent( entity, std::move( transform ) );

		eage::ecs::PhysicsComponent physics;
		physics.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
		physics.sync_transform_from_body = true;
		physics.is_bullet = true;
		physics.QueueSetActive( false );
		mRegistry.AddComponent( entity, std::move( physics ) );

		eage::ecs::CircleColliderComponent collider;
		collider.radius = config.collider_radius;
		collider.is_sensor = true;
		collider.category_bits = config.category_bits;
		collider.mask_bits = config.mask_bits;
		collider.group_index = -2;
		mRegistry.AddComponent( entity, std::move( collider ) );

		mRegistry.AddComponent( entity, BulletComponent{ BulletState::Inactive, config.damage, config.lifetime_sec } );

		render_system.AttachRenderable( entity, mesh_id, config.material_id, texture_index );

		if( config.clip_id != eage::ecs::INVALID_ID )
		{
			mAnimationSystem.Attach( entity, config.clip_id );
		}
	}

	return pool_id;
}

bool
BulletSystem::Fire( BulletPoolId pool_id, glm::vec2 position, glm::vec2 direction, float speed )
{
	auto pool_it = mPools.find( pool_id );
	if( pool_it == mPools.end() || pool_it->second.empty() )
	{
		return false;
	}

	float interval = mPoolFireInterval[pool_id];
	if( interval > 0.f )
	{
		auto now = std::chrono::steady_clock::now();
		auto last_it = mPoolLastFireTime.find( pool_id );
		if( last_it != mPoolLastFireTime.end() )
		{
			float elapsed = std::chrono::duration<float>( now - last_it->second ).count();
			if( elapsed < interval )
			{
				return false;
			}
		}
		mPoolLastFireTime[pool_id] = now;
	}

	auto& pool = pool_it->second;
	uint64_t entity = pool.front();
	pool.pop_front();

	auto& bullet = mRegistry.GetComponent<BulletComponent>( entity );
	bullet.state = BulletState::Alive;
	bullet.age_sec = 0.f;

	auto& render = mRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render.visible = true;

	if( mAnimationSystem.HasAnimation( entity ) )
	{
		mAnimationSystem.ShowFrame( entity, 0 );
		mAnimationSystem.Pause( entity );
	}

	auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
	transform.SetPosition( glm::vec3( position, 0.f ) );

	const float rotation_angle = rotation_angle_from_up( direction );
	transform.SetRotation( glm::angleAxis( rotation_angle, glm::vec3( 0.f, 0.f, 1.f ) ) );

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	physics.QueueSetActive( true );
	physics.QueueSetPosition( position );
	physics.QueueSetRotation( rotation_angle );
	physics.QueueSetVelocity( direction * speed );
	return true;
}

void
BulletSystem::Update( float dt )
{
	std::vector<uint64_t> bullets_to_despawn;

	for( auto [entity, bullet] : mRegistry.GetComponentMap<BulletComponent>() )
	{
		if( bullet.state == BulletState::Dying )
		{
			if( !mAnimationSystem.HasAnimation( entity ) || mAnimationSystem.IsFinished( entity ) )
			{
				bullets_to_despawn.push_back( entity );
			}
			continue;
		}

		if( bullet.state != BulletState::Alive )
		{
			continue;
		}

		auto& bullet_cmp = mRegistry.GetComponent<BulletComponent>( entity );
		bullet_cmp.age_sec += dt;

		if( bullet_cmp.lifetime_sec > 0.f && bullet_cmp.age_sec >= bullet_cmp.lifetime_sec )
		{
			DespawnBullet( entity );
		}
	}

	for( uint64_t entity : bullets_to_despawn )
	{
		DespawnBullet( entity );
	}
}

void
BulletSystem::OnSensorEnter( uint64_t sensor, uint64_t visitor )
{
	if( !mRegistry.HasComponent<BulletComponent>( sensor ) )
	{
		return;
	}

	auto& bullet = mRegistry.GetComponent<BulletComponent>( sensor );
	if( bullet.state != BulletState::Alive )
	{
		return;
	}

	if( mRegistry.HasComponent<HealthComponent>( visitor ) )
	{
		mRegistry.GetComponent<HealthComponent>( visitor ).pending_damage += bullet.damage;
	}

	BeginHitReaction( sensor );
}

void
BulletSystem::OnSensorExit( uint64_t sensor, uint64_t visitor )
{
}

void
BulletSystem::OnCollideBegin( uint64_t entityA, uint64_t entityB )
{
}

void
BulletSystem::OnCollideEnd( uint64_t entityA, uint64_t entityB )
{
}

void
BulletSystem::BeginHitReaction( uint64_t entity )
{
	auto& bullet = mRegistry.GetComponent<BulletComponent>( entity );
	bullet.state = BulletState::Dying;

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	physics.QueueSetVelocity( glm::vec2( 0.f, 0.f ) );
	physics.QueueSetActive( false );

	if( mAnimationSystem.HasAnimation( entity ) )
	{
		mAnimationSystem.PlayOnce( entity, 0 );
	}
	else
	{
		DespawnBullet( entity );
	}
}

void
BulletSystem::DespawnBullet( uint64_t entity )
{
	LOG() << "Despawning bullet entity " << entity;
	auto& bullet = mRegistry.GetComponent<BulletComponent>( entity );
	bullet.state = BulletState::Inactive;
	bullet.age_sec = 0.f;

	auto& render = mRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render.visible = false;

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	glm::vec2 inactive_pos = mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, -INACTIVE_OFFSET );
	physics.QueueSetPosition( inactive_pos );
	physics.QueueSetActive( false );

	if( mAnimationSystem.HasAnimation( entity ) )
	{
		mAnimationSystem.ShowFrame( entity, 0 );
		mAnimationSystem.Pause( entity );
	}

	auto pool_it = mEntityToPool.find( entity );
	if( pool_it != mEntityToPool.end() )
	{
		mPools[pool_it->second].push_back( entity );
	}
}
