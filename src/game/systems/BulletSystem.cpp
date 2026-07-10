#include "BulletSystem.h"

#include <vector>

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <utility/Logger.h>

#include "../GameConfig.h"
#include "../components/BulletComponent.h"
#include "../components/HealthComponent.h"

using namespace vortex;
using namespace utility;

namespace
{
	constexpr float INACTIVE_OFFSET = 2000.0f;
}

BulletSystem::BulletSystem( eage::ecs::ECSRegistry& registry, eage::ecs::PhysicsSystem& physics_system )
	: mRegistry( registry )
	, mPhysicsSystem( physics_system )
{
	mPhysicsSystem.Subscribe( this );

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) * 0.5f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) * 0.5f;
	mScreenTopLeft = glm::vec2( -half_width, half_height );
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

	if( config.animation && config.animation->GetFrameCount() > 0 )
	{
		const glm::ivec2 frame_size = config.animation->GetFrameSize();
		if( frame_size.x > 0 && frame_size.y > 0 )
		{
			mesh_width = static_cast<float>( frame_size.x );
			mesh_height = static_cast<float>( frame_size.y );
		}
		texture_index = config.animation->GetFrameTexture( 0 );
	}

	eage::ecs::ResourceId mesh_id = render_system.CreateSpriteMesh( mesh_width, mesh_height );

	for( int i = 0; i < count; ++i )
	{
		auto entity = mRegistry.CreateEntity();
		pool.push_back( entity );
		mEntityToPool[entity] = pool_id;

		auto& root = mRegistry.GetComponent<eage::ecs::SceneGraphComponent>( root_entity );
		root.children_entities.push_back( entity );
		eage::ecs::SceneGraphComponent relationship;
		relationship.parent_entity = root_entity;
		mRegistry.AddComponent( entity, std::move( relationship ) );

		eage::ecs::TransformComponent transform;
		transform.SetPosition( glm::vec3( inactive_pos, 0.f ) );
		mRegistry.AddComponent( entity, std::move( transform ) );

		eage::ecs::PhysicsComponent physics;
		physics.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
		physics.sync_transform_from_body = true;
		physics.is_bullet = true;
		physics.QueueSleep( true );
		mRegistry.AddComponent( entity, std::move( physics ) );

		eage::ecs::CircleColliderComponent collider;
		collider.radius = config.collider_radius;
		collider.is_sensor = true;
		collider.category_bits = config.category_bits;
		collider.mask_bits = config.mask_bits;
		collider.group_index = -2;
		mRegistry.AddComponent( entity, std::move( collider ) );

		mRegistry.AddComponent( entity, BulletComponent{ BulletState::Inactive, config.damage } );

		render_system.AttachRenderable( entity, mesh_id, config.material_id, texture_index );

		if( config.animation && config.animation->GetFrameCount() > 0 )
		{
			auto [sprite_it, _] = mBulletSprites.try_emplace(
				entity,
				config.animation,
				entity,
				mRegistry );
			sprite_it->second.ShowFrame( 0 );
			sprite_it->second.Pause();
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

	auto& render = mRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render.visible = true;

	if( auto sprite_it = mBulletSprites.find( entity ); sprite_it != mBulletSprites.end() )
	{
		sprite_it->second.ShowFrame( 0 );
		sprite_it->second.Pause();
	}

	auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
	transform.SetPosition( glm::vec3( position, 0.f ) );

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	physics.QueueSetPosition( position );
	physics.QueueSetVelocity( direction * speed );
	physics.QueueSleep( false );
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
			auto sprite_it = mBulletSprites.find( entity );
			if( sprite_it != mBulletSprites.end() )
			{
				sprite_it->second.Update( dt );
				if( sprite_it->second.IsFinished() )
				{
					bullets_to_despawn.push_back( entity );
				}
			}
			else
			{
				bullets_to_despawn.push_back( entity );
			}
			continue;
		}

		if( bullet.state != BulletState::Alive )
		{
			continue;
		}

		if( !mRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) )
		{
			continue;
		}

		auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
		float x = transform.position.x;
		float y = transform.position.y;

		bool out_of_bounds = x < mScreenTopLeft.x || x > mScreenBottomRight.x ||
							 y < mScreenTopLeft.y || y < mScreenBottomRight.y;
		if( out_of_bounds )
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
	physics.QueueSleep( true );

	if( auto sprite_it = mBulletSprites.find( entity ); sprite_it != mBulletSprites.end() )
	{
		sprite_it->second.PlayOnce( 0 );
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

	auto& render = mRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render.visible = false;

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	glm::vec2 inactive_pos = mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, -INACTIVE_OFFSET );
	physics.QueueSetPosition( inactive_pos );
	physics.QueueSleep( true );

	if( auto sprite_it = mBulletSprites.find( entity ); sprite_it != mBulletSprites.end() )
	{
		sprite_it->second.ShowFrame( 0 );
		sprite_it->second.Pause();
	}

	auto pool_it = mEntityToPool.find( entity );
	if( pool_it != mEntityToPool.end() )
	{
		mPools[pool_it->second].push_back( entity );
	}
}
