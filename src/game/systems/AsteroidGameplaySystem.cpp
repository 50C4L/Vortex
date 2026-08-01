#include "AsteroidGameplaySystem.h"

#include <assets/SceneResourceLoader.h>
#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/EffectSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <graphics/MaterialBuilder.h>
#include <utility/Logger.h>

#include <cstdlib>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../GameConfig.h"
#include "../components/RewardComponent.h"
#include "../components/ExperienceComponent.h"
#include "../components/GameGenericComponents.h"
#include "../components/HealthComponent.h"
#include "../components/PlayerComponents.h"

using namespace vortex;
using namespace utility;

namespace
{
	constexpr float INACTIVE_OFFSET = 333.0f; // Offset to move inactive asteroids off-screen
	constexpr float SPAWN_AREA_PADDING = 33.0f; // Padding from screen edges for spawning asteroids
}

AsteroidGameplaySystem::AsteroidGameplaySystem( eage::ecs::ECSRegistry& registry,
												eage::ecs::PhysicsSystem& physics_system,
												eage::ecs::EffectSystem& effect_system )
	: mECSRegistry( registry )
	, mPhysicsSystem( physics_system )
	, mEffectSystem( effect_system )
{
	mPhysicsSystem.Subscribe( this );
	mScreenTopLeft = glm::vec2( config::layout::PLAY_FIELD_LEFT, config::layout::PLAY_FIELD_TOP );
	mScreenBottomRight = glm::vec2( config::layout::PLAY_FIELD_RIGHT, config::layout::PLAY_FIELD_BOTTOM );
}

AsteroidGameplaySystem::~AsteroidGameplaySystem()
{
	mPhysicsSystem.Unsubscribe( this );
}

void
AsteroidGameplaySystem::SetDeathEffect( eage::ecs::ResourceId effect_id )
{
	mDeathEffectId = effect_id;
}

void
AsteroidGameplaySystem::PrepareAsteroids( eage::ecs::RenderSystem& render_system, assets::SceneResourceLoader& resources, int count, uint64_t root_entity )
{
	// Create asteroid material and texture
	const uint32_t asteroid_texture = resources.GetTexture( "./resources/textures/asteroid/Asteroid L.png" );

	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.SetAlphaBlending( true )
		.EnableDepthTest( true )
		.Build();

	mAsteroidMaterialId = render_system.CreateMaterial( material_property );
	mAsteroidTextureIndex = asteroid_texture;

	// Create shared mesh - ALL ASTEROIDS CAN USE THIS
	mAsteroidMeshId = render_system.CreateSpriteMesh( 32.f, 32.f );

	// Create given number of asteroids
	for( int i = 0; i < count; ++i )
	{
		auto asteroid = mECSRegistry.CreateEntity();
		mAvailableAsteroids.push_back( asteroid );

		// Set parent-child relationship with scene root
		auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponent>( root_entity );
		root.children_entities.push_back( asteroid );
		eage::ecs::SceneGraphComponent relationship;
		relationship.parent_entity = root_entity;
		mECSRegistry.AddComponent( asteroid, std::move( relationship ) );
		
		// Transform component, initial position off-screen right-bottom corner + offset
		eage::ecs::TransformComponent transform;
		transform.SetPosition( glm::vec3( mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, INACTIVE_OFFSET * -1.f ), 0.0f ) );
		mECSRegistry.AddComponent( asteroid, std::move( transform ) );

		// Physics component
		eage::ecs::PhysicsComponent physics_cmp;
		physics_cmp.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
		physics_cmp.sync_transform_from_body = true;
		physics_cmp.max_linear_velocity = 150.0f;
		physics_cmp.QueueSleep( true ); // Start asleep
		mECSRegistry.AddComponent( asteroid, std::move( physics_cmp ) );

		eage::ecs::CircleColliderComponent collider;
		collider.radius = 16.f; // Approximate radius of the asteroid
		// collider.is_sensor = true;
		collider.category_bits = config::PHYSX_CAT_WARPABLE | config::PHYSX_CAT_ENEMY;
		collider.mask_bits = config::PHYSX_CAT_SCREEN_ZONE | config::PHYSX_CAT_PLAYER | config::PHYSX_CAT_BULLET;
		collider.group_index = -1; // Negative group index = never collide with same group
		mECSRegistry.AddComponent( asteroid, std::move( collider ) );

		// Gameplay components
		mECSRegistry.AddComponent( asteroid, WarpComponent{} );
		mECSRegistry.AddComponent( asteroid, HealthComponent{ 1.f, 1.f, 0.f } );
		mECSRegistry.AddComponent( asteroid, RewardComponent{ 2 } );

		// Track all asteroid entities for Update()
		mAllAsteroids.insert( asteroid );

		// Render component
		render_system.AttachRenderable( asteroid, mAsteroidMeshId, mAsteroidMaterialId, mAsteroidTextureIndex, false );
	}
}

void
AsteroidGameplaySystem::SpawnAsteroid( int count )
{
	for( int i = 0; i < count; ++i )
	{
		if( mAvailableAsteroids.empty() )
		{
			break; // No more available asteroids to spawn
		}
		auto asteroid = mAvailableAsteroids.front();
		mAvailableAsteroids.pop_front();

		auto& render_cmp = mECSRegistry.GetComponent<eage::ecs::RenderComponent>( asteroid );
		render_cmp.visible = true;

		// Spawn at random position just outside screen bounds + SPAWN_AREA_PADDING
		auto& transform = mECSRegistry.GetComponent<eage::ecs::TransformComponent>( asteroid );
		float x_pos = 0.0f;
		float y_pos = 0.0f;
		int side = rand() % 4; // 0: left, 1: right, 2: top, 3: bottom
		switch( side )
		{
			case 0: // Left
			{
				x_pos = mScreenTopLeft.x - SPAWN_AREA_PADDING;
				y_pos = mScreenTopLeft.y + static_cast<float>( rand() % static_cast<int>( (mScreenBottomRight.y - mScreenTopLeft.y) ) );
			}
				break;
			case 1: // Right
			{
				x_pos = mScreenBottomRight.x + SPAWN_AREA_PADDING;
				y_pos = mScreenTopLeft.y + static_cast<float>( rand() % static_cast	<int>( (mScreenBottomRight.y - mScreenTopLeft.y) ) );
			}
				break;
			case 2: // Top
			{
				x_pos = mScreenTopLeft.x + static_cast<float>( rand() % static_cast<int>( (mScreenBottomRight.x - mScreenTopLeft.x) ) );
				y_pos = mScreenTopLeft.y + SPAWN_AREA_PADDING;
			}
				break;
			case 3: // Bottom
			{
				x_pos = mScreenTopLeft.x + static_cast<float>( rand() % static_cast<int>( (mScreenBottomRight.x - mScreenTopLeft.x) ) );
				y_pos = mScreenBottomRight.y - SPAWN_AREA_PADDING;
			}
				break;
		}
		transform.SetPosition( glm::vec3( x_pos, y_pos, 0.0f ) );

		// Physics
		auto& physics_cmp = mECSRegistry.GetComponent<eage::ecs::PhysicsComponent>( asteroid );
		physics_cmp.QueueSetPosition( glm::vec2( x_pos, y_pos ) );
		// Set a constant velocity towards a random point on screen
		glm::vec2 target_point;
		target_point.x = mScreenTopLeft.x + static_cast<float>( rand() % static_cast<int>( (mScreenBottomRight.x - mScreenTopLeft.x) ) );
		target_point.y = mScreenBottomRight.y + static_cast<float>( rand() % static_cast<int>( (mScreenTopLeft.y - mScreenBottomRight.y) ) );
		glm::vec2 direction = glm::normalize( target_point - glm::vec2( transform.position.x, transform.position.y ) );
		float speed = 50.0f + static_cast<float>( rand() % 100 ); // Random speed between 50 and 150
		physics_cmp.QueueAddVelocity( direction * speed );
		// Random angular velocity
		float angular_speed = (rand() % 20) - 10.f; // Random angular speed between -10 and 10
		physics_cmp.QueueSetAngularVelocity( angular_speed );
		// Wake up physics body 
		physics_cmp.QueueSleep( false ); // Wake up

		// Reset health for reuse
		auto& health = mECSRegistry.GetComponent<HealthComponent>( asteroid );
		health.health = health.max_health;
		health.pending_damage = 0.f;
	}
}

void
AsteroidGameplaySystem::DespawnAsteroid( uint64_t asteroid_entity )
{
	if( !mECSRegistry.HasComponent<eage::ecs::RenderComponent>( asteroid_entity ) ||
		!mECSRegistry.HasComponent<eage::ecs::TransformComponent>( asteroid_entity ) ||
		!mECSRegistry.HasComponent<eage::ecs::PhysicsComponent>( asteroid_entity ) )
	{
		LOG() << "Attempted to despawn invalid asteroid entity: " << asteroid_entity;
		return; // Not a valid asteroid entity
	}

	// Disable rendering
	auto& render_cmp = mECSRegistry.GetComponent<eage::ecs::RenderComponent>( asteroid_entity );
	render_cmp.visible = false;

	// Disable physics and move off-screen
	auto& physics_cmp = mECSRegistry.GetComponent<eage::ecs::PhysicsComponent>( asteroid_entity );
	// Stop all movement
	physics_cmp.QueueSetPosition( mScreenBottomRight + glm::vec2( INACTIVE_OFFSET, INACTIVE_OFFSET * -1.f ) );
	physics_cmp.QueueSleep( true );

	mAvailableAsteroids.push_back( asteroid_entity );
}

void 
AsteroidGameplaySystem::Update()
{
	for( uint64_t entity : mAllAsteroids )
	{
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

			for( auto [ player_entity, player ] : mECSRegistry.GetComponentMap<PlayerComponent>() )
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

			DespawnAsteroid( entity );
		}
	}
}

void
AsteroidGameplaySystem::OnSensorEnter( uint64_t sensor, uint64_t visitor )
{
}

void
AsteroidGameplaySystem::OnSensorExit( uint64_t sensor, uint64_t visitor )
{
}

void
AsteroidGameplaySystem::OnCollideBegin( uint64_t entityA, uint64_t entityB )
{
	auto applyDamage = [&]( uint64_t entity )
	{
		if( mECSRegistry.HasComponent<PlayerComponent>( entity ) &&
			mECSRegistry.HasComponent<HealthComponent>( entity ) )
		{
			mECSRegistry.GetComponent<HealthComponent>( entity ).pending_damage += 50.f;
		}
	};

	applyDamage( entityA );
	applyDamage( entityB );
}

void
AsteroidGameplaySystem::OnCollideEnd( uint64_t entityA, uint64_t entityB )
{
}
