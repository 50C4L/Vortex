#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Camera.h>
#include <events/InputController.h>
#include <audio/AudioMixer.h>
#include <imgui/imgui.h>

#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/components/Hud.h>

#include "GameConfig.h"
#include "systems/AsteroidGameplaySystem.h"
#include <graphics/MaterialBuilder.h>
#include "systems/BulletSystem.h"
#include "systems/PlayerInputSystem.h"
#include "systems/PlayerGameplaySystem.h"
#include "systems/WarpSystem.h"
#include "components/GameGenericComponents.h"

using namespace vortex;
using namespace vortex::config;
using namespace utility;

MainScene::MainScene( const EngineContext& ctx )
	: mInputController( ctx.input )
	, mECSRegistry( ctx.registry )
	, mAudioSystem( ctx.audio_system )
	, mRenderSystem( ctx.render_system )
	, mPhysicsSystem( ctx.physics_system )
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

	PrepareAnimations();

	PrepareMeshes();

	PrepareMaterials();

	CreateSceneRoot();

	CreateBackgroundEntity();

	CreatePlayerEntity();

	CreateScreenZoneEntities();

	CreateEnemyEntities();

	CreateHudEntities();

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
MainScene::Update( float dt )
{
	// System update
	mPlayerGameplaySystem->Update( dt );
	mBulletSystem->Update( dt );
	mAsteroidGameplaySystem->Update();

	// Update HUD kill counter
	if( mKillCountHudEntity != 0 )
	{
		auto& text_cmp = mECSRegistry.GetComponent<eage::ecs::HudTextComponent>( mKillCountHudEntity );
		text_cmp.text = "Kills: " + std::to_string( mAsteroidGameplaySystem->GetKillCount() );
	}

	// Update camera
	mRenderSystem.SetCamera( *mCamera, glm::vec2(
		static_cast<float>( config::VirtualResolution::WIDTH ),
		static_cast<float>( config::VirtualResolution::HEIGHT ) ) );
}

void
MainScene::PrepareAnimations()
{
	mDefaultBulletClip = assets::AnimationClip::Load(
		mRenderSystem,
		"./resources/textures/bullets/anim_defaultBullet/animation.json" );

	if( mDefaultBulletClip->GetFrameCount() == 0 )
	{
		LOG_ERROR( "MainScene: failed to load default bullet animation" );
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
	mECSRegistry.AddComponent( mSceneRootEntity, eage::ecs::SceneGraphComponent{} );
}

void
MainScene::CreateBackgroundEntity()
{
	const uint32_t background_texture = mRenderSystem.CreateTexture( "./resources/textures/background/dark.png" );

	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.SetAlphaBlending( true )
		.EnableDepthTest( true )
		.Build();

	auto material_id = mRenderSystem.CreateMaterial( material_property );

	auto mesh_id = mRenderSystem.CreateSpriteMesh( 1280.f, 720.f );

	// Create entity
	mBackgroundEntity = mECSRegistry.CreateEntity();

	// Parent to scene root
	auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponent>( mSceneRootEntity );
	root.children_entities.push_back( mBackgroundEntity );
	eage::ecs::SceneGraphComponent relationship;
	relationship.parent_entity = mSceneRootEntity;
	mECSRegistry.AddComponent( mBackgroundEntity, std::move( relationship ) );

	// Transform -- centered at origin.
	// z=-1 places the background behind the gameplay plane (z=0). With conventional depth
	// (LESS_OR_EQUAL, clear=1), objects further from the camera have larger depth values and
	// correctly lose the depth test against closer objects.
	eage::ecs::TransformComponent transform;
	transform.SetPosition( glm::vec3( 0.f, 0.f, -1.f ) );
	mECSRegistry.AddComponent( mBackgroundEntity, std::move( transform ) );

	// Attach renderable
	mRenderSystem.AttachRenderable( mBackgroundEntity, mesh_id, material_id, background_texture );
}

void
MainScene::CreatePlayerEntity()
{
	mPlayerInputSystem = std::make_unique<PlayerInputSystem>( mECSRegistry, mInputController );
	mPlayerGameplaySystem = std::make_unique<PlayerGameplaySystem>( mECSRegistry, *mBulletSystem, mAudioSystem );
	mPlayerGameplaySystem->PreparePlayer( mRenderSystem, mSceneRootEntity, *mDefaultBulletClip );
}

void 
MainScene::CreateScreenZoneEntities()
{
	mOnScreenZoneEntity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with scene root
	auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponent>( mSceneRootEntity );
	root.children_entities.push_back( mOnScreenZoneEntity );
	eage::ecs::SceneGraphComponent screen_zone_relationship;
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
	mBulletSystem = std::make_unique<BulletSystem>( mECSRegistry, mPhysicsSystem );
	mAsteroidGameplaySystem = std::make_unique<AsteroidGameplaySystem>( mECSRegistry, mPhysicsSystem );
}

void
MainScene::CreateEnemyEntities()
{
	mAsteroidGameplaySystem->PrepareAsteroids( mRenderSystem, 100, mSceneRootEntity );

	mAsteroidGameplaySystem->SpawnAsteroid( 10 );
}

void
MainScene::CreateHudEntities()
{
	mKillCountHudEntity = mECSRegistry.CreateEntity();

	eage::ecs::HudTransformComponent hud_tf;
	hud_tf.position = glm::vec2( 1.0f, 0.0f );
	hud_tf.offset_px = glm::vec2( -7.0f, 5.0f );
	hud_tf.anchor = eage::ecs::HudAnchor::TOP_RIGHT;
	mECSRegistry.AddComponent( mKillCountHudEntity, std::move( hud_tf ) );

	eage::ecs::HudTextComponent text_cmp;
	text_cmp.text = "Kills: 0";
	text_cmp.font_size = eage::ecs::HudFontSize::LARGE;
	text_cmp.color = glm::vec4( 0.0f, 0.83f, 1.0f, 1.0f );
	mECSRegistry.AddComponent( mKillCountHudEntity, std::move( text_cmp ) );
}
