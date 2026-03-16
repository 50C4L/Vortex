#include "BulletSystem.h"

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

BulletSystem::BulletSystem( eage::ecs::ECSRegistry& registry, eage::ecs::RenderSystem& render_system,
							eage::ecs::PhysicsSystem& physics_system )
	: mRegistry( registry )
	, mRenderSystem( render_system )
	, mPhysicsSystem( physics_system )
{
	mPhysicsSystem.RegisterObserver( this );

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) * 0.5f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) * 0.5f;
	mScreenTopLeft = glm::vec2( -half_width, half_height );
	mScreenBottomRight = glm::vec2( half_width, -half_height );
}

BulletSystem::~BulletSystem()
{
	mPhysicsSystem.UnregisterObserver( this );
}

BulletPoolId
BulletSystem::PreparePool( const BulletPoolConfig& config, int count, uint64_t root_entity )
{
	BulletPoolId pool_id = mNextPoolId++;
	auto& pool = mPools[pool_id];

	glm::vec2 inactive_pos = mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, -INACTIVE_OFFSET );

	// Create a shared mesh for this pool
	eage::ecs::ResourceId mesh_id = mRenderSystem.CreateSpriteMesh(
		config.mesh_width, config.mesh_height,
		glm::vec2( 0.f, 0.f ), glm::vec2( 1.f, 1.f ) );

	for( int i = 0; i < count; ++i )
	{
		auto entity = mRegistry.CreateEntity();
		pool.push_back( entity );
		mEntityToPool[entity] = pool_id;

		// Scene graph: child of world root
		auto& root = mRegistry.GetComponent<eage::ecs::SceneGraphComponment>( root_entity );
		root.children_entities.push_back( entity );
		eage::ecs::SceneGraphComponment relationship;
		relationship.parent_entity = root_entity;
		mRegistry.AddComponent( entity, std::move( relationship ) );

		// Transform: start off-screen
		eage::ecs::TransformComponent transform;
		transform.SetPosition( glm::vec3( inactive_pos, 0.f ) );
		mRegistry.AddComponent( entity, std::move( transform ) );

		// Physics: dynamic, CCD enabled, starts asleep
		eage::ecs::PhysicsComponent physics;
		physics.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
		physics.sync_transform_from_body = true;
		physics.is_bullet = true;
		physics.QueueSleep( true );
		mRegistry.AddComponent( entity, std::move( physics ) );

		// Collider: sensor so it generates hit events without physics response
		eage::ecs::CircleColliderComponent collider;
		collider.radius = config.collider_radius;
		collider.is_sensor = true;
		collider.category_bits = config.category_bits;
		collider.mask_bits = config.mask_bits;
		collider.group_index = -2;
		mRegistry.AddComponent( entity, std::move( collider ) );

		// Gameplay tag
		mRegistry.AddComponent( entity, BulletComponent{ false, config.damage } );

		// Render
		mRenderSystem.AttachSprite( entity, config.material_id, config.mesh_width, config.mesh_height, config.uv_min, config.uv_max );
	}

	return pool_id;
}

void
BulletSystem::Fire( BulletPoolId pool_id, glm::vec2 position, glm::vec2 direction, float speed )
{
	auto pool_it = mPools.find( pool_id );
	if( pool_it == mPools.end() || pool_it->second.empty() )
	{
		return; // Pool exhausted or invalid - silent skip
	}

	auto& pool = pool_it->second;
	uint64_t entity = pool.front();
	pool.pop_front();

	auto& bullet = mRegistry.GetComponent<BulletComponent>( entity );
	bullet.is_alive = true;

	auto& render = mRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render.visible = true;

	// Update TransformComponent immediately so BulletSystem::Update()'s OOB check
	// sees the spawn position in the same frame (QueueSetPosition only applies next physics tick).
	auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
	transform.SetPosition( glm::vec3( position, 0.f ) );

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	physics.QueueSetPosition( position );
	physics.QueueSetVelocity( direction * speed );
	physics.QueueSleep( false );
}

void
BulletSystem::Update()
{
	for( auto& [entity, bullet] : mRegistry.GetComponentMap<BulletComponent>() )
	{
		if( !bullet.is_alive )
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
							 y > mScreenTopLeft.y || y < mScreenBottomRight.y;
		if( out_of_bounds )
		{
			DespawnBullet( entity );
		}
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
	if( !bullet.is_alive )
	{
		return;
	}

	// Apply damage to whatever was hit
	if( mRegistry.HasComponent<HealthComponent>( visitor ) )
	{
		mRegistry.GetComponent<HealthComponent>( visitor ).pending_damage += bullet.damage;
	}

	DespawnBullet( sensor );
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
BulletSystem::DespawnBullet( uint64_t entity )
{
	LOG() << "Despawning bullet entity " << entity;
	auto& bullet = mRegistry.GetComponent<BulletComponent>( entity );
	bullet.is_alive = false;

	auto& render = mRegistry.GetComponent<eage::ecs::RenderComponent>( entity );
	render.visible = false;

	auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
	glm::vec2 inactive_pos = mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, -INACTIVE_OFFSET );
	physics.QueueSetPosition( inactive_pos );
	physics.QueueSleep( true );

	auto pool_it = mEntityToPool.find( entity );
	if( pool_it != mEntityToPool.end() )
	{
		mPools[pool_it->second].push_back( entity );
	}
}
