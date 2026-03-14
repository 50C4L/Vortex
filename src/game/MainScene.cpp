#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <graphics/Camera.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>
#include <graphics/Material.h>
#include <graphics/MaterialBuilder.h>
#include <imgui/imgui.h>
#include <events/InputController.h>
#include <audio/AudioMixer.h>
#include <assets/ImageLoader.h>
#include <assets/TextureAtlas.h>

#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>

#include "GameConfig.h"
#include "systems/AsteroidGameplaySystem.h"
#include "systems/PlayerInputSystem.h"
#include "systems/PlayerGameplaySystem.h"
#include "systems/WarpSystem.h"
#include "components/GameGenericComponents.h"
#include "components/HealthComponent.h"
#include "components/PlayerComponents.h"

using namespace vortex;
using namespace vortex::config;
using namespace utility;

namespace
{
	struct SceneGlobalData
	{
		alignas(64) glm::mat4 view;
		alignas(64) glm::mat4 proj;
		alignas(64) glm::mat4 view_proj;
		// padding
		float extra[16];
	};
}

MainScene::MainScene( eage::graphics::Renderer& renderer, events::InputController& input_controller,
					  eage::ecs::ECSRegistry& ecs_registry, eage::ecs::AudioSystem& audio_system, eage::ecs::RenderSystem& render_system,
					  eage::ecs::PhysicsSystem& physics_system )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mECSRegistry( ecs_registry )
	, mAudioSystem( audio_system )
	, mRenderSystem( render_system )
	, mPhysicsSystem( physics_system )
	, mLastUpdateTime( std::chrono::high_resolution_clock::now() )
{
}

MainScene::~MainScene()
{
}

void
MainScene::OnEnter()
{
	LOG( "MainScene::OnEnter" );

	InitializeGenericSystems();

	PrepareMeshes();

	PrepareMaterials();

	CreateSceneRoot();

	CreatePlayerEntity();

	CreateScreenZoneEntities();

	CreateEnemyEntities();

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) / 2.f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) / 2.f;
	mCamera = std::make_shared<eage::graphics::OrthographicCamera>( half_width * -1.f, half_width, half_height * -1.f, half_height, 0.1f, 100.0f );
	mCamera->SetPosition( { 0, 0, 2.f } );
}

uint64_t
MainScene::GetSceneRoot()
{
	return mSceneRootEntity;
}

void
MainScene::OnExit()
{
	LOG( "MainScene::OnExit" );
}

void
MainScene::Update()
{
	std::chrono::time_point<std::chrono::steady_clock> current_time = std::chrono::steady_clock::now();
	std::chrono::duration<float, std::milli> delta_time_ms = current_time - mLastUpdateTime;
	mLastUpdateTime = current_time;

	// player input
	mPlayerGameplaySystem->Update( delta_time_ms.count() / 1000.f );

	// Update camera
	auto current_frame = mRenderer.GetCurrentFrameIndex();
	{
		SceneGlobalData scene_global_data;
		scene_global_data.view = mCamera->GetViewMatrix();
		scene_global_data.proj = mCamera->GetProjectionMatrix();
		scene_global_data.view_proj = scene_global_data.proj * scene_global_data.view;
		mRenderSystem.GetGlobalUniformBuffer()->Update( &scene_global_data, sizeof( SceneGlobalData ), sizeof( SceneGlobalData ) * current_frame );
	}
}

void 
MainScene::PrepareMeshes()
{
}

void
MainScene::PrepareMaterials()
{
	// Create player material
	mRenderSystem.CreateImageBuffer( "./resources/textures/ship/ship_texatlas.png" );

	// Create sprite material using the new MaterialBuilder and RenderSystem
	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders("./src/shaders/compiled/colored_triangle_mesh.vert.spv", 
					"./src/shaders/compiled/colored_triangle.frag.spv")
		.AddTexture(0, "./resources/textures/ship/ship_texatlas.png", 
					vk::Filter::eNearest, vk::Filter::eNearest)
		.SetAlphaBlending()
		.EnableDepthTest(true)
		.Build();

	mPlayerMaterialId = mRenderSystem.CreateMaterial(material_property);
}

void 
MainScene::CreateSceneRoot()
{
	mSceneRootEntity = mECSRegistry.CreateEntity();

	// Root identity transform
	mECSRegistry.AddComponent( mSceneRootEntity, eage::ecs::TransformComponent{} );

	// Root relationship component
	mECSRegistry.AddComponent( mSceneRootEntity, eage::ecs::SceneGraphComponment{} );
}

void
MainScene::CreatePlayerEntity()
{
	mPlayerInputSystem = std::make_unique<PlayerInputSystem>( mECSRegistry, mInputController );
	mPlayerGameplaySystem = std::make_unique<PlayerGameplaySystem>( mECSRegistry );

	mPlayerEntity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with scene root
	auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( mSceneRootEntity );
	root.children_entities.push_back( mPlayerEntity );
	eage::ecs::SceneGraphComponment player_relationship;
	player_relationship.parent_entity = mSceneRootEntity;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player_relationship ) );

	// Player component with all player-specific data
	PlayerComponent player;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player ) );

	// Health component
	mECSRegistry.AddComponent( mPlayerEntity, HealthComponent{} );
	
	// Transform component
	mECSRegistry.AddComponent( mPlayerEntity, eage::ecs::TransformComponent{} );

	// Physics component
	eage::ecs::PhysicsComponent player_physics;
	player_physics.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
	// player_physics.is_sensor = true;
	player_physics.sync_transform_from_body = true;
	player_physics.max_linear_velocity = 400.0f;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player_physics ) );

	eage::ecs::CircleColliderComponent player_collider;
	player_collider.radius = 25.f; // Approximate radius of the ship
	player_collider.category_bits = PHYSX_CAT_WARPABLE | PHYSX_CAT_PLAYER;
	player_collider.mask_bits = PHYSX_CAT_SCREEN_ZONE;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player_collider ) );

	// Render component
	assets::TextureAtlas texture_atlas( "./resources/textures/ship/ship_texatlas.json" );
	texture_atlas.Flip();
	const auto& ship_tex = texture_atlas.GetSubTexture( "player_ship.png" );
	mRenderSystem.AttachSprite( mPlayerEntity, mPlayerMaterialId, 50.f, 50.f, ship_tex.uv_min, ship_tex.uv_max );

	// Audio components
	AudioSourceComponent thrust_audio;
	thrust_audio.sound_path = "./resources/sounds/thruster.mp3";
	thrust_audio.sound_resource_id = mAudioSystem.LoadSound( thrust_audio.sound_path );
	thrust_audio.should_loop = true;
	mECSRegistry.AddComponent( mPlayerEntity, std::move(thrust_audio) );
	mECSRegistry.AddComponent( mPlayerEntity, AudioEventComponent{}) ;

	// Gameplay components
	mECSRegistry.AddComponent( mPlayerEntity, WarpComponent{} );

	// Create Thruster entity - child of ship
	auto thruster_entity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with player entity
	auto& player_scene = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( mPlayerEntity );
	player_scene.children_entities.push_back( thruster_entity );
	auto& player_cmp = mECSRegistry.GetComponent<PlayerComponent>( mPlayerEntity );
	player_cmp.thruster_fx_entity = thruster_entity;

	// Thruster transform component
	eage::ecs::TransformComponent thruster_transform;
	thruster_transform.SetPosition( glm::vec3(0.0f, -30.f, 0.0f) ); // Behind ship in local space
	thruster_transform.SetScale( glm::vec3( 1 / 5.f ) );
	mECSRegistry.AddComponent( thruster_entity, std::move(thruster_transform) );

	const auto& thrust_tex = texture_atlas.GetSubTexture( "ship_thrust_fx.png" );
	mRenderSystem.AttachSprite( thruster_entity, mPlayerMaterialId, 50.f, 50.f, thrust_tex.uv_min, thrust_tex.uv_max );
}

void 
MainScene::CreateScreenZoneEntities()
{
	mOnScreenZoneEntity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with scene root
	auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( mSceneRootEntity );
	root.children_entities.push_back( mOnScreenZoneEntity );
	eage::ecs::SceneGraphComponment screen_zone_relationship;
	screen_zone_relationship.parent_entity = mSceneRootEntity;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( screen_zone_relationship ) );

	// Add Transform component
	mECSRegistry.AddComponent( mOnScreenZoneEntity, eage::ecs::TransformComponent{} );

	// Collision component
	eage::ecs::PhysicsComponent physics;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( physics ) );

	// Box collider component
	eage::ecs::BoxColliderComponent box_collider;
	box_collider.width = static_cast<float>( config::DesignResolution::WIDTH );
	box_collider.height = static_cast<float>( config::DesignResolution::HEIGHT );
	box_collider.is_sensor = true;
	box_collider.category_bits = PHYSX_CAT_SCREEN_ZONE;
	box_collider.mask_bits = PHYSX_CAT_WARPABLE;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( box_collider ) );

	// Gameplay component
	WarpBoundaryComponent warp_boundary;
	warp_boundary.left = -static_cast<float>( config::DesignResolution::WIDTH ) * 0.5f;
	warp_boundary.right = static_cast<float>( config::DesignResolution::WIDTH ) * 0.5f;
	warp_boundary.top = static_cast<float>( config::DesignResolution::HEIGHT ) * 0.5f;
	warp_boundary.bottom = -static_cast<float>( config::DesignResolution::HEIGHT ) * 0.5f;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( warp_boundary ) );

	mWarpSystem->SetScreenEntity( mOnScreenZoneEntity );
}

void
MainScene::InitializeGenericSystems()
{
	mWarpSystem = std::make_unique<WarpSystem>( mECSRegistry, mPhysicsSystem );
	mAsteroidGameplaySystem = std::make_unique<AsteroidGameplaySystem>( mECSRegistry, mRenderSystem, mPhysicsSystem );
}

void
MainScene::CreateEnemyEntities()
{
	mAsteroidGameplaySystem->PrepareAsteroids( 100, mSceneRootEntity );

	mAsteroidGameplaySystem->SpawnAsteroid( 5 );
}
