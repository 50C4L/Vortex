#include "EnemySystem.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include <assets/SceneResourceLoader.h>
#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/EffectSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/systems/SceneGraphSystem.h>
#include <graphics/MaterialBuilder.h>
#include <utility/Logger.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../GameConfig.h"
#include "../components/DriftComponent.h"
#include "../components/EnemyComponent.h"
#include "../components/ExperienceComponent.h"
#include "../components/GameGenericComponents.h"
#include "../components/HealthComponent.h"
#include "../components/PlayerComponents.h"
#include "../components/RewardComponent.h"

using namespace vortex;
using namespace utility;

namespace
{
	constexpr float INACTIVE_OFFSET = 333.0f;
	constexpr float SPAWN_AREA_PADDING = 33.0f;
}

EnemySystem::EnemySystem( eage::ecs::ECSRegistry& registry,
						  eage::ecs::PhysicsSystem& physics_system,
						  eage::ecs::EffectSystem& effect_system,
						  eage::ecs::SceneGraphSystem& scene_graph_system )
	: mECSRegistry( registry )
	, mPhysicsSystem( physics_system )
	, mEffectSystem( effect_system )
	, mSceneGraphSystem( scene_graph_system )
{
	mPhysicsSystem.Subscribe( this );
	mScreenTopLeft = glm::vec2( config::layout::PLAY_FIELD_LEFT, config::layout::PLAY_FIELD_TOP );
	mScreenBottomRight = glm::vec2( config::layout::PLAY_FIELD_RIGHT, config::layout::PLAY_FIELD_BOTTOM );
}

EnemySystem::~EnemySystem()
{
	mPhysicsSystem.Unsubscribe( this );
}

void
EnemySystem::SetDeathEffect( eage::ecs::ResourceId effect_id )
{
	mDeathEffectId = effect_id;
}

void
EnemySystem::PreparePool( eage::ecs::RenderSystem& render_system,
						  assets::SceneResourceLoader& resources,
						  const EnemyDefinition& definition,
						  int count,
						  uint64_t root_entity )
{
	if( definition.id.empty() )
	{
		LOG_ERROR() << "EnemySystem::PreparePool: definition id is empty";
		return;
	}

	if( mPools.find( definition.id ) != mPools.end() )
	{
		LOG_ERROR() << "EnemySystem::PreparePool: pool already exists for " << definition.id;
		return;
	}

	if( count <= 0 )
	{
		LOG_ERROR() << "EnemySystem::PreparePool: count must be positive for " << definition.id;
		return;
	}

	if( !resources.HasTexture( definition.texture_path ) )
	{
		LOG_ERROR() << "EnemySystem::PreparePool: texture not in catalog: " << definition.texture_path;
		return;
	}

	const uint32_t texture_index = resources.GetTexture( definition.texture_path );

	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.SetAlphaBlending( true )
		.EnableDepthTest( true )
		.Build();

	EnemyPool pool;
	pool.material = render_system.CreateMaterial( material_property );
	pool.mesh = render_system.CreateSpriteMesh( definition.sprite_width, definition.sprite_height );

	const glm::vec3 inactive_pos(
		mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, INACTIVE_OFFSET * -1.f ),
		0.0f );

	for( int i = 0; i < count; ++i )
	{
		auto entity = mECSRegistry.CreateEntity();
		pool.available.push_back( entity );
		pool.all.insert( entity );
		mEntityToPool[entity] = definition.id;

		mSceneGraphSystem.AddNodeToParent( entity, root_entity );

		eage::ecs::TransformComponent transform;
		transform.SetPosition( inactive_pos );
		mECSRegistry.AddComponent( entity, std::move( transform ) );

		eage::ecs::PhysicsComponent physics_cmp;
		physics_cmp.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
		physics_cmp.sync_transform_from_body = true;
		physics_cmp.max_linear_velocity = definition.max_linear_velocity;
		physics_cmp.QueueSetActive( false );
		mECSRegistry.AddComponent( entity, std::move( physics_cmp ) );

		eage::ecs::CircleColliderComponent collider;
		collider.radius = definition.collider_radius;
		collider.category_bits = config::PHYSX_CAT_ENEMY;
		collider.mask_bits = config::PHYSX_CAT_SCREEN_ZONE | config::PHYSX_CAT_PLAYER | config::PHYSX_CAT_BULLET;
		collider.group_index = -1;
		if( definition.warpable )
		{
			collider.category_bits |= config::PHYSX_CAT_WARPABLE;
		}
		mECSRegistry.AddComponent( entity, std::move( collider ) );

		mECSRegistry.AddComponent( entity, EnemyComponent{
			definition.id, definition.behavior, definition.contact_damage } );
		mECSRegistry.AddComponent( entity, HealthComponent{ definition.max_health, definition.max_health, 0.f } );
		mECSRegistry.AddComponent( entity, RewardComponent{ definition.xp_reward } );

		if( definition.warpable )
		{
			mECSRegistry.AddComponent( entity, WarpComponent{} );
		}

		switch( definition.behavior )
		{
			case EnemyBehavior::DRIFT:
				mECSRegistry.AddComponent( entity, DriftComponent{
					definition.drift.speed_min,
					definition.drift.speed_max,
					definition.drift.angular_speed_max } );
				break;
		}

		render_system.AttachRenderable( entity, pool.mesh.Get(), pool.material.Get(), texture_index, false );
	}

	mPools.emplace( definition.id, std::move( pool ) );
}

void
EnemySystem::ReleaseAll()
{
	for( const auto& [entity, pool_id] : mEntityToPool )
	{
		(void)pool_id;
		mECSRegistry.QueueDestroyEntity( entity );
	}

	mPools.clear();
	mEntityToPool.clear();
	mDeathEffectId = eage::ecs::INVALID_ID;
}

bool
EnemySystem::Spawn( const std::string& definition_id, int count )
{
	auto pool_it = mPools.find( definition_id );
	if( pool_it == mPools.end() )
	{
		LOG_ERROR() << "EnemySystem::Spawn: no pool for " << definition_id;
		return false;
	}

	EnemyPool& pool = pool_it->second;
	int spawned = 0;

	for( int i = 0; i < count; ++i )
	{
		if( pool.available.empty() )
		{
			break;
		}

		auto entity = pool.available.front();
		pool.available.pop_front();

		auto& render_cmp = mECSRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
		render_cmp.visible = true;

		const glm::vec2 spawn_pos = PickRandomEdgePosition();
		auto& transform = mECSRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
		transform.SetPosition( glm::vec3( spawn_pos, 0.0f ) );

		auto& physics_cmp = mECSRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
		physics_cmp.QueueSetActive( true );
		physics_cmp.QueueSetPosition( spawn_pos );

		if( mECSRegistry.HasComponent<DriftComponent>( entity ) )
		{
			ApplyDriftSpawn( entity );
		}

		if( mECSRegistry.HasComponent<HealthComponent>( entity ) )
		{
			auto& health = mECSRegistry.GetComponent<HealthComponent>( entity );
			health.health = health.max_health;
			health.pending_damage = 0.f;
		}

		++spawned;
	}

	return spawned == count;
}

void
EnemySystem::DespawnAll()
{
	std::vector<uint64_t> live_entities;
	for( const auto& [id, pool] : mPools )
	{
		(void)id;
		for( uint64_t entity : pool.all )
		{
			if( std::find( pool.available.begin(), pool.available.end(), entity ) == pool.available.end() )
			{
				live_entities.push_back( entity );
			}
		}
	}

	for( uint64_t entity : live_entities )
	{
		Despawn( entity );
	}
}

int
EnemySystem::GetLiveCount() const
{
	int live = 0;
	for( const auto& [id, pool] : mPools )
	{
		(void)id;
		live += static_cast<int>( pool.all.size() ) - static_cast<int>( pool.available.size() );
	}
	return live;
}

void
EnemySystem::Despawn( uint64_t entity )
{
	auto pool_it = mEntityToPool.find( entity );
	if( pool_it == mEntityToPool.end() )
	{
		LOG() << "Attempted to despawn unknown enemy entity: " << entity;
		return;
	}

	if( !mECSRegistry.HasComponent<eage::ecs::RenderComponent>( entity ) ||
		!mECSRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) ||
		!mECSRegistry.HasComponent<eage::ecs::PhysicsComponent>( entity ) )
	{
		LOG() << "Attempted to despawn invalid enemy entity: " << entity;
		return;
	}

	auto& render_cmp = mECSRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render_cmp.visible = false;

	auto& physics_cmp = mECSRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	physics_cmp.QueueSetPosition( mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, INACTIVE_OFFSET * -1.f ) );
	physics_cmp.QueueSetActive( false );

	mPools[pool_it->second].available.push_back( entity );
}

void
EnemySystem::ApplyDriftSpawn( uint64_t entity )
{
	const auto& drift = mECSRegistry.GetComponent<DriftComponent>( entity );
	auto& transform = mECSRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
	auto& physics_cmp = mECSRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );

	glm::vec2 target_point;
	target_point.x = mScreenTopLeft.x + static_cast<float>(
		rand() % static_cast<int>( ( mScreenBottomRight.x - mScreenTopLeft.x ) ) );
	target_point.y = mScreenBottomRight.y + static_cast<float>(
		rand() % static_cast<int>( ( mScreenTopLeft.y - mScreenBottomRight.y ) ) );

	glm::vec2 to_target = target_point - glm::vec2( transform.position.x, transform.position.y );
	if( glm::length( to_target ) <= 0.001f )
	{
		to_target = glm::vec2( 0.f, 1.f );
	}
	glm::vec2 direction = glm::normalize( to_target );

	float speed = drift.speed_min;
	const int speed_range = static_cast<int>( drift.speed_max - drift.speed_min );
	if( speed_range > 0 )
	{
		speed += static_cast<float>( rand() % ( speed_range + 1 ) );
	}
	physics_cmp.QueueAddVelocity( direction * speed );

	float angular_speed = 0.f;
	const int angular_span = static_cast<int>( drift.angular_speed_max * 2.f );
	if( angular_span > 0 )
	{
		angular_speed = static_cast<float>( rand() % ( angular_span + 1 ) ) - drift.angular_speed_max;
	}
	physics_cmp.QueueSetAngularVelocity( angular_speed );
}

glm::vec2
EnemySystem::PickRandomEdgePosition() const
{
	float x_pos = 0.0f;
	float y_pos = 0.0f;
	const int side = rand() % 4;
	switch( side )
	{
		case 0: // Left
		{
			x_pos = mScreenTopLeft.x - SPAWN_AREA_PADDING;
			y_pos = mScreenTopLeft.y + static_cast<float>(
				rand() % static_cast<int>( ( mScreenBottomRight.y - mScreenTopLeft.y ) ) );
		}
			break;
		case 1: // Right
		{
			x_pos = mScreenBottomRight.x + SPAWN_AREA_PADDING;
			y_pos = mScreenTopLeft.y + static_cast<float>(
				rand() % static_cast<int>( ( mScreenBottomRight.y - mScreenTopLeft.y ) ) );
		}
			break;
		case 2: // Top
		{
			x_pos = mScreenTopLeft.x + static_cast<float>(
				rand() % static_cast<int>( ( mScreenBottomRight.x - mScreenTopLeft.x ) ) );
			y_pos = mScreenTopLeft.y + SPAWN_AREA_PADDING;
		}
			break;
		case 3: // Bottom
		{
			x_pos = mScreenTopLeft.x + static_cast<float>(
				rand() % static_cast<int>( ( mScreenBottomRight.x - mScreenTopLeft.x ) ) );
			y_pos = mScreenBottomRight.y - SPAWN_AREA_PADDING;
		}
			break;
	}

	return { x_pos, y_pos };
}

void
EnemySystem::Update()
{
	if( IsPaused() )
	{
		return;
	}

	for( auto [entity, enemy] : mECSRegistry.GetComponentMap<EnemyComponent>() )
	{
		(void)enemy;

		if( !mECSRegistry.HasComponent<HealthComponent>( entity ) )
		{
			continue;
		}

		auto& health = mECSRegistry.GetComponent<HealthComponent>( entity );
		if( health.IsDead() || health.pending_damage <= 0.f )
		{
			continue;
		}

		health.health -= health.pending_damage;
		health.pending_damage = 0.f;

		if( health.IsDead() )
		{
			const int reward_xp = mECSRegistry.HasComponent<RewardComponent>( entity )
				? mECSRegistry.GetComponent<RewardComponent>( entity ).xp
				: 0;

			for( auto [player_entity, player] : mECSRegistry.GetComponentMap<PlayerComponent>() )
			{
				++player.kill_count;

				if( reward_xp > 0 && mECSRegistry.HasComponent<ExperienceComponent>( player_entity ) )
				{
					mECSRegistry.GetComponent<ExperienceComponent>( player_entity ).pending_xp += reward_xp;
				}
			}

			if( mDeathEffectId != eage::ecs::INVALID_ID &&
				mECSRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) )
			{
				const auto& transform = mECSRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
				const float angle = ( static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ) ) * glm::two_pi<float>();
				const glm::quat rotation = glm::angleAxis( angle, glm::vec3( 0.f, 0.f, 1.f ) );
				mEffectSystem.Apply( mDeathEffectId, glm::vec2( transform.position.x, transform.position.y ), rotation );
			}

			Despawn( entity );
		}
	}
}

void
EnemySystem::ApplyContactDamage( uint64_t maybe_enemy, uint64_t maybe_player )
{
	if( !mECSRegistry.HasComponent<EnemyComponent>( maybe_enemy ) ||
		!mECSRegistry.HasComponent<PlayerComponent>( maybe_player ) ||
		!mECSRegistry.HasComponent<HealthComponent>( maybe_player ) )
	{
		return;
	}

	const auto& enemy = mECSRegistry.GetComponent<EnemyComponent>( maybe_enemy );
	mECSRegistry.GetComponent<HealthComponent>( maybe_player ).pending_damage += enemy.contact_damage;
}

void
EnemySystem::OnSensorEnter( uint64_t sensor, uint64_t visitor )
{
	(void)sensor;
	(void)visitor;
}

void
EnemySystem::OnSensorExit( uint64_t sensor, uint64_t visitor )
{
	(void)sensor;
	(void)visitor;
}

void
EnemySystem::OnCollideBegin( uint64_t entityA, uint64_t entityB )
{
	ApplyContactDamage( entityA, entityB );
	ApplyContactDamage( entityB, entityA );
}

void
EnemySystem::OnCollideEnd( uint64_t entityA, uint64_t entityB )
{
	(void)entityA;
	(void)entityB;
}
