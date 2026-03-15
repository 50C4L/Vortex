#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <graphics/Camera.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>
#include <imgui/imgui.h>
#include <events/InputController.h>
#include <audio/AudioMixer.h>
#include <assets/ImageLoader.h>

#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>

#include "GameConfig.h"
#include "systems/AsteroidGameplaySystem.h"
#include "systems/BulletSystem.h"
#include "systems/PlayerInputSystem.h"
#include "systems/PlayerGameplaySystem.h"
#include "systems/WarpSystem.h"
#include "components/GameGenericComponents.h"

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

	mBulletSystem->Update();

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
	mPlayerGameplaySystem = std::make_unique<PlayerGameplaySystem>( mECSRegistry, *mBulletSystem, mRenderSystem, mAudioSystem );
	mPlayerGameplaySystem->PreparePlayer( mSceneRootEntity );
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
	mBulletSystem = std::make_unique<BulletSystem>( mECSRegistry, mRenderSystem, mPhysicsSystem );
	mAsteroidGameplaySystem = std::make_unique<AsteroidGameplaySystem>( mECSRegistry, mRenderSystem, mPhysicsSystem );
}

void
MainScene::CreateEnemyEntities()
{
	mAsteroidGameplaySystem->PrepareAsteroids( 100, mSceneRootEntity );

	mAsteroidGameplaySystem->SpawnAsteroid( 10 );
}
