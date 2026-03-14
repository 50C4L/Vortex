#include "AsteroidGameplaySystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <graphics/MaterialBuilder.h>
#include <assets/TextureAtlas.h>
#include <utility/Logger.h>

#include "../GameConfig.h"
#include "../components/GameGenericComponents.h"
#include "../components/HealthComponent.h"
#include "../components/PlayerComponents.h"

using namespace vortex;
using namespace utility;

namespace
{
	constexpr float INACTIVE_OFFSET = 1000.0f; // Offset to move inactive asteroids off-screen
	constexpr float SPAWN_AREA_PADDING = 100.0f; // Padding from screen edges for spawning asteroids
}

AsteroidGameplaySystem::AsteroidGameplaySystem( eage::ecs::ECSRegistry& registry, eage::ecs::RenderSystem& render_system,
												eage::ecs::PhysicsSystem& physics_system )
	: mECSRegistry( registry )
	, mRenderSystem( render_system )
	, mPhysicsSystem( physics_system )
{
	mPhysicsSystem.RegisterObserver( this );
	float half_width = static_cast<float>( config::DesignResolution::WIDTH) * 0.5f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT) * 0.5f;
	mScreenTopLeft = glm::vec2( -half_width, half_height );
	mScreenBottomRight = glm::vec2( half_width, -half_height );
}

AsteroidGameplaySystem::~AsteroidGameplaySystem()
{
	mPhysicsSystem.UnregisterObserver( this );
}

void
AsteroidGameplaySystem::PrepareAsteroids( int count, uint64_t root_entity )
{
	// Create asteroid material
	mRenderSystem.CreateImageBuffer( "./resources/textures/asteroid/asteroid.png" );

	// Create sprite material using the new MaterialBuilder and RenderSystem
	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders("./src/shaders/compiled/colored_triangle_mesh.vert.spv", 
					"./src/shaders/compiled/colored_triangle.frag.spv")
		.AddTexture(0, "./resources/textures/asteroid/asteroid.png", 
					vk::Filter::eNearest, vk::Filter::eNearest)
		.SetAlphaBlending()
		.EnableDepthTest(true)
		.Build();

	mAsteroidMaterialId = mRenderSystem.CreateMaterial( material_property );

	// Load texture
	assets::TextureAtlas texture_atlas( "./resources/textures/asteroid/asteroid.json" );
	texture_atlas.Flip();
	const auto& asteroid_tex = texture_atlas.GetSubTexture( "asteroid_L_0.png" );

	// Create shared mesh - ALL ASTEROIDS CAN USE THIS
	mAsteroidMeshId = mRenderSystem.CreateSpriteMesh( 50.f, 50.f, asteroid_tex.uv_min, asteroid_tex.uv_max );

	// Create given number of asteroids
	for( int i = 0; i < count; ++i )
	{
		auto asteroid = mECSRegistry.CreateEntity();
		mAvailableAsteroids.push_back( asteroid );

		// Set parent-child relationship with scene root
		auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( root_entity );
		root.children_entities.push_back( asteroid );
		eage::ecs::SceneGraphComponment relationship;
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
		physics_cmp.max_linear_velocity = 300.0f;
		physics_cmp.QueueSleep( true ); // Start asleep
		mECSRegistry.AddComponent( asteroid, std::move( physics_cmp ) );

		eage::ecs::CircleColliderComponent collider;
		collider.radius = 25.f; // Approximate radius of the ship
		// collider.is_sensor = true;
		collider.category_bits = config::PHYSX_CAT_WARPABLE | config::PHYSX_CAT_ENEMY;
		collider.mask_bits = config::PHYSX_CAT_SCREEN_ZONE | config::PHYSX_CAT_PLAYER;
		collider.group_index = -1; // Negative group index = never collide with same group
		mECSRegistry.AddComponent( asteroid, std::move( collider ) );

		// Gameplay components
		mECSRegistry.AddComponent( asteroid, WarpComponent{} );

		// Render component
		mRenderSystem.AttachRenderable( asteroid, mAsteroidMeshId, mAsteroidMaterialId, false );
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
		float speed = 100.0f + static_cast<float>( rand() % 200 ); // Random speed between 100 and 300
		physics_cmp.QueueAddVelocity( direction * speed );
		// Random angular velocity
		float angular_speed = (rand() % 20) - 10.f; // Random angular speed between -10 and 10
		physics_cmp.QueueSetAngularVelocity( angular_speed );
		// Wake up physics body 
		physics_cmp.QueueSleep( false ); // Wake up
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
