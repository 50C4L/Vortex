#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Camera.h>
#include <graphics/Renderer.h>
#include <graphics/SceneRenderPass.h>
#include <graphics/CompositePass.h>
#include <events/InputController.h>
#include <audio/AudioMixer.h>

#include <ecs/systems/AnimationSystem.h>
#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/EffectSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/systems/SceneGraphSystem.h>
#include <assets/SceneResourceLoader.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>

#include "GameConfig.h"
#include "systems/AsteroidGameplaySystem.h"
#include <graphics/MaterialBuilder.h>
#include "systems/BulletSystem.h"
#include "systems/LevelingSystem.h"
#include "systems/PlayerInputSystem.h"
#include "systems/PlayerGameplaySystem.h"
#include "systems/WarpSystem.h"
#include "components/GameGenericComponents.h"
#include "ui/StatusPanel.h"

#include <ui/UISystem.h>
#include <ui/UIView.h>

using namespace vortex;
using namespace vortex::config;
using namespace utility;

MainScene::MainScene( const EngineContext& ctx )
	: mRenderer( ctx.renderer )
	, mUISystem( ctx.ui_system )
	, mInputController( ctx.input )
	, mECSRegistry( ctx.registry )
	, mAudioSystem( ctx.audio_system )
	, mAnimationSystem( ctx.animation_system )
	, mEffectSystem( ctx.effect_system )
	, mRenderSystem( ctx.render_system )
	, mPhysicsSystem( ctx.physics_system )
	, mSceneGraphSystem( ctx.scene_graph_system )
	, mResourceLoader( ctx.resource_loader )
{
}

MainScene::~MainScene()
{
}

void
MainScene::OnEnter()
{
	LOG( "MainScene::OnEnter" );

	mScenePass = std::make_unique<eage::graphics::SceneRenderPass>(
		mRenderer,
		static_cast<uint32_t>( config::VirtualResolution::WIDTH ),
		static_cast<uint32_t>( config::VirtualResolution::HEIGHT ) );
	mRenderer.AddRenderPass( mScenePass.get() );
	mRenderSystem.SetScenePass( mScenePass.get() );

	// Fonts (and other assets) must be loaded before RmlUi parses the document.
	if( !mResourceLoader.LoadManifest( "./resources/scenes/main_scene.json" ) )
	{
		LOG_ERROR( "MainScene: failed to load resource manifest" );
	}

	mUIView = std::make_unique<eage::ui::UIView>(
		mUISystem,
		"main_hud",
		static_cast<uint32_t>( config::VirtualResolution::WIDTH ),
		static_cast<uint32_t>( config::VirtualResolution::HEIGHT ) );
	mStatusPanel = std::make_unique<StatusPanel>( mECSRegistry, mUIView->GetDataModel() );
	if( !mUIView->LoadDocument( "./src/game/ui/rml/hud.rml" ) )
	{
		LOG_ERROR( "MainScene: failed to load HUD document" );
	}
	mRenderer.AddRenderPass( &mUIView->GetRenderPass() );

	mCompositePass = std::make_unique<eage::graphics::CompositePass>(
		mRenderer,
		static_cast<uint32_t>( config::VirtualResolution::WIDTH ),
		static_cast<uint32_t>( config::VirtualResolution::HEIGHT ) );
	mCompositePass->SetInputs( &mScenePass->GetColorTarget(), mUIView->GetOutput() );
	mRenderer.AddRenderPass( mCompositePass.get() );

	InitializeGenericSystems();

	PrepareMeshes();

	PrepareMaterials();

	CreateSceneRoot();

	CreateBackgroundEntity();

	CreateExplosionEffect();

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

	if( mBulletSystem )
	{
		mBulletSystem->ReleaseAll();
	}
	if( mAsteroidGameplaySystem )
	{
		mAsteroidGameplaySystem->ReleaseAll();
	}
	mEffectSystem.ReleaseAll();
	if( mPlayerGameplaySystem )
	{
		mPlayerGameplaySystem->ReleaseAll();
	}

	if( mBackgroundEntity != 0 )
	{
		mECSRegistry.QueueDestroyEntity( mBackgroundEntity );
		mBackgroundEntity = 0;
	}
	if( mOnScreenZoneEntity != 0 )
	{
		mECSRegistry.QueueDestroyEntity( mOnScreenZoneEntity );
		mOnScreenZoneEntity = 0;
	}
	if( mSceneRootEntity != 0 )
	{
		mECSRegistry.QueueDestroyEntity( mSceneRootEntity );
		mSceneRootEntity = 0;
	}

	mECSRegistry.FlushDestroyQueue();
	mSceneGraphSystem.SetSceneRoot( 0 );

	mEffectMaterial.Reset();
	mExplosionEffectId = 0;

	mPlayerInputSystem.reset();
	mPlayerGameplaySystem.reset();
	mWarpSystem.reset();
	mAsteroidGameplaySystem.reset();
	mBulletSystem.reset();
	mLevelingSystem.reset();

	mRenderer.WaitForIdle();
	mRenderSystem.FlushPendingDeletes();

	if( mCompositePass )
	{
		mRenderer.RemoveRenderPass( mCompositePass.get() );
		mCompositePass.reset();
	}

	if( mUIView )
	{
		mRenderer.RemoveRenderPass( &mUIView->GetRenderPass() );
	}
	mStatusPanel.reset();
	mUIView.reset();

	mRenderSystem.SetScenePass( nullptr );
	if( mScenePass )
	{
		mRenderer.RemoveRenderPass( mScenePass.get() );
		mScenePass.reset();
	}
}

eage::graphics::ManagedImage*
MainScene::GetOutput()
{
	if( mCompositePass )
	{
		return mCompositePass->GetColorTarget();
	}
	if( mScenePass )
	{
		return mScenePass->GetDesc().color_target;
	}
	return nullptr;
}

void
MainScene::Update( float dt )
{
	// System update
	mPlayerGameplaySystem->Update( dt );
	mBulletSystem->Update( dt );
	mAsteroidGameplaySystem->Update();
	mLevelingSystem->Update();

	if( mStatusPanel )
	{
		mStatusPanel->Update();
	}

	// Update camera
	mRenderSystem.SetCamera( *mCamera, glm::vec2(
		static_cast<float>( config::VirtualResolution::WIDTH ),
		static_cast<float>( config::VirtualResolution::HEIGHT ) ) );
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
	mECSRegistry.AddComponent( mSceneRootEntity, eage::ecs::SceneGraphComponent{} );

	mSceneGraphSystem.SetSceneRoot( mSceneRootEntity );
}

void
MainScene::CreateBackgroundEntity()
{
	const uint32_t background_texture = mResourceLoader.GetTexture( "./resources/textures/background/dark.png" );

	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.SetAlphaBlending( true )
		.EnableDepthTest( true )
		.Build();

	auto material = mRenderSystem.CreateMaterial( material_property );
	auto mesh = mRenderSystem.CreateSpriteMesh( 1280.f, 720.f );

	// Create entity
	mBackgroundEntity = mECSRegistry.CreateEntity();
	mSceneGraphSystem.AddNodeToParent( mBackgroundEntity, mSceneRootEntity );

	// Transform -- centered at origin.
	// z=-1 places the background behind the gameplay plane (z=0). With conventional depth
	// (LESS_OR_EQUAL, clear=1), objects further from the camera have larger depth values and
	// correctly lose the depth test against closer objects.
	eage::ecs::TransformComponent transform;
	transform.SetPosition( glm::vec3( 0.f, 0.f, -1.f ) );
	mECSRegistry.AddComponent( mBackgroundEntity, std::move( transform ) );

	// Attach renderable; Reset handles so the entity is the sole owner.
	mRenderSystem.AttachRenderable( mBackgroundEntity, mesh.Get(), material.Get(), background_texture );
	mesh.Reset();
	material.Reset();
}

void
MainScene::CreatePlayerEntity()
{
	mPlayerInputSystem = std::make_unique<PlayerInputSystem>( mECSRegistry, mInputController );
	mPlayerGameplaySystem = std::make_unique<PlayerGameplaySystem>( mECSRegistry, *mBulletSystem, mAudioSystem, mSceneGraphSystem );
	mPlayerGameplaySystem->PreparePlayer( mRenderSystem, mSceneRootEntity, mResourceLoader );
}

void 
MainScene::CreateScreenZoneEntities()
{
	mOnScreenZoneEntity = mECSRegistry.CreateEntity();
	mSceneGraphSystem.AddNodeToParent( mOnScreenZoneEntity, mSceneRootEntity );

	// Add Transform component
	mECSRegistry.AddComponent( mOnScreenZoneEntity, eage::ecs::TransformComponent{} );

	// Collision component
	eage::ecs::PhysicsComponent physics;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( physics ) );

	// Box collider component -- narrower than the render target so objects wrap
	// before sliding under the status panel. Offset centres the box on the play field.
	eage::ecs::BoxColliderComponent box_collider;
	box_collider.width = layout::PLAY_FIELD_WIDTH;
	box_collider.height = layout::PLAY_FIELD_HEIGHT;
	box_collider.offset = { layout::PLAY_FIELD_CENTER_X, 0.f };
	box_collider.is_sensor = true;
	box_collider.category_bits = PHYSX_CAT_SCREEN_ZONE;
	box_collider.mask_bits = PHYSX_CAT_WARPABLE;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( box_collider ) );

	// Gameplay component
	WarpBoundaryComponent warp_boundary;
	warp_boundary.left = layout::PLAY_FIELD_LEFT;
	warp_boundary.right = layout::PLAY_FIELD_RIGHT;
	warp_boundary.top = layout::PLAY_FIELD_TOP;
	warp_boundary.bottom = layout::PLAY_FIELD_BOTTOM;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( warp_boundary ) );

	mWarpSystem->SetScreenEntity( mOnScreenZoneEntity );
}

void
MainScene::InitializeGenericSystems()
{
	mWarpSystem = std::make_unique<WarpSystem>( mECSRegistry, mPhysicsSystem );
	mBulletSystem = std::make_unique<BulletSystem>( mECSRegistry, mPhysicsSystem, mAnimationSystem, mSceneGraphSystem );
	mAsteroidGameplaySystem = std::make_unique<AsteroidGameplaySystem>( mECSRegistry, mPhysicsSystem, mEffectSystem, mSceneGraphSystem );
	mLevelingSystem = std::make_unique<LevelingSystem>( mECSRegistry );
}

void
MainScene::CreateExplosionEffect()
{
	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.SetAlphaBlending( true )
		.EnableDepthTest( true )
		.Build();

	mEffectMaterial = mRenderSystem.CreateMaterial( material_property );

	eage::ecs::EffectSystem::EffectConfig config;
	config.clip_ids = { mResourceLoader.GetClip( "./resources/textures/effects/anim_explosion1/animation.json" ) };
	config.material_id = mEffectMaterial.Get();
	config.sound_id = mResourceLoader.GetSound( "./resources/sounds/explosion1.wav" );
	config.pool_size = 32;
	mExplosionEffectId = mEffectSystem.Create( mRenderSystem, config, mSceneRootEntity );
}

void
MainScene::CreateEnemyEntities()
{
	mAsteroidGameplaySystem->PrepareAsteroids( mRenderSystem, mResourceLoader, 100, mSceneRootEntity );
	mAsteroidGameplaySystem->SetDeathEffect( mExplosionEffectId );
	mAsteroidGameplaySystem->SpawnAsteroid( 10 );
}
