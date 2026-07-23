#include "VortexGame.h"
#include "EngineContext.h"

#include <chrono>
#include <iostream>

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <utility/Filesystem.h>
#include <graphics/Renderer.h>
#include <graphics/SceneRenderPass.h>
#include <graphics/ImGuiRenderPass.h>
#include <graphics/ImGuiHudRenderer.h>
#include <graphics/ManagedVulkanResources.h>
#include <events/InputController.h>
#include <events/KeyCode.h>
#include <audio/AudioMixer.h>
#include <ecs/ECS.h>
#include <ecs/systems/AnimationSystem.h>
#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/EffectSystem.h>
#include <ecs/systems/PhysicsSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/systems/SceneGraphSystem.h>
#include <ecs/systems/HudRenderSystem.h>
#include <profiling/PerformanceTracker.h>

#include "SceneController.h"
#include "game/MainScene.h"
#include "game/GameConfig.h"
#include "game/UIStyle.h"

using namespace vortex;
using namespace vortex::config;

VortexGame::VortexGame()
{
}

void
VortexGame::OnSceneChanged( uint64_t scene_root )
{
	if( mSceneGraphSystem )
	{
		mSceneGraphSystem->SetSceneRoot( scene_root );
	}
}

VortexGame::~VortexGame()
{
	// GPU resources must be released before the renderer (which owns VMA/device).
	// Systems like RenderSystem hold VMA-allocated buffers and images, so they
	// must be torn down while the allocator is still alive.
	mRenderer->WaitForIdle();

	mPerformanceTracker.reset();
	mHudRenderSystem.reset();
	mImGuiHudRenderer.reset();
	mRenderSystem.reset();
	mSceneGraphSystem.reset();
	mPhysicsSystem.reset();
	mEffectSystem.reset();
	mAnimationSystem.reset();
	mAudioSystem.reset();
	mSceneController.reset();

	mImGuiPass.reset();
	mScenePass.reset();
	mRenderer.reset();
	mWindow.reset();
	SDL_Quit();
}

void
VortexGame::Run()
{
	auto last_frame_time = std::chrono::steady_clock::now();
	bool quit = false;
	while( !quit )
	{
		auto now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration<float>( now - last_frame_time ).count();
		last_frame_time = now;
		SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
			{
				quit = true;
			}

			mImGuiPass->ProcessEvent( event );

			if( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
			{
				mInputController->Handle( event );
			}
		}

		// Update performance tracker
		mPerformanceTracker->Update();

		mSceneController->Update( dt );

		mPhysicsSystem->Update( dt );
		mAnimationSystem->Update( dt );
		mEffectSystem->Update();
		mAudioSystem->Update( dt );
		mSceneGraphSystem->Update();
		mRenderSystem->Update();
		mRenderer->Render();
	}
	mRenderer->WaitForIdle();
	mSceneController->FreeAllScenes();
}

bool
VortexGame::Init()
{
	utility::GetLogger().RegisterThread( std::this_thread::get_id(), "Main" );
	utility::init_content_working_directory();

	// Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		return false;
	}

	config::ScreenResolution screen_res;
	mWindow = utility::make_resource( 
		SDL_CreateWindow, SDL_DestroyWindow,
		"Vortex Game",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_res.width, screen_res.height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );
	if( !mWindow )
	{
		std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
		return false;
	}

	// Initialize Renderer
	mRenderer = std::make_unique<eage::graphics::Renderer>( *mWindow );
	if( !mRenderer->Init() )
	{
		std::cerr << "Failed to initialize Renderer" << std::endl;
		return false;
	}

	// Create render passes
	mScenePass = std::make_unique<eage::graphics::SceneRenderPass>(
		*mRenderer,
		static_cast<uint32_t>( config::VirtualResolution::WIDTH ),
		static_cast<uint32_t>( config::VirtualResolution::HEIGHT ) );
	mImGuiPass = std::make_unique<eage::graphics::ImGuiRenderPass>(
		mRenderer->GetVulkanContext(),
		*mWindow,
		mRenderer->GetSwapchainFormat(),
		eage::graphics::Renderer::MAX_FRAMES_IN_FLIGHT,
		mRenderer->GetSwapchainImageCount(),
		mScenePass->GetColorTarget() );
	float ui_scale = config::get_scale_factor( screen_res.width, static_cast<int>( config::DesignResolution::WIDTH ) );
	mImGuiPass->LoadFont( nullptr, 13.0f * ui_scale, eage::ecs::HudFontSize::SMALL );
	mImGuiPass->LoadFont( nullptr, 24.0f * ui_scale, eage::ecs::HudFontSize::MEDIUM );
	mImGuiPass->LoadFont( nullptr, 36.0f * ui_scale, eage::ecs::HudFontSize::LARGE );
	mImGuiPass->InitFontTexture( [this]( std::function<void( vk::CommandBuffer& )> work )
	{
		mRenderer->ImmediateSubmit( std::move( work ) );
	} );
	apply_game_style();

	mRenderer->AddRenderPass( mScenePass.get() );
	mRenderer->AddRenderPass( mImGuiPass.get() );

	// Initialize AudioMixer
	mAudioMixer = std::make_unique<eage::audio::AudioMixer>();

	// Initialize InputController
	std::unordered_map<events::KeyCode, uint64_t> keycode_to_event = {
		{ events::KeyCode::A, static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ) },
		{ events::KeyCode::D, static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ) },
		{ events::KeyCode::W, static_cast<uint64_t>( GameEvents::PLAYER_THRUST ) },
		{ events::KeyCode::J, static_cast<uint64_t>( GameEvents::PLAYER_SHOOT ) },
	};
	mInputController = std::make_unique<events::InputController>( std::move( keycode_to_event ) );

	// Initialize ECSRegistry
	mECSRegistry = std::make_unique<eage::ecs::ECSRegistry>();

	// Initialize AudioSystem
	mAudioSystem = std::make_unique<eage::ecs::AudioSystem>( *mECSRegistry, *mAudioMixer );

	// Initialize AnimationSystem
	mAnimationSystem = std::make_unique<eage::ecs::AnimationSystem>( *mECSRegistry );

	// Initialize EffectSystem
	mEffectSystem = std::make_unique<eage::ecs::EffectSystem>( *mECSRegistry, *mAnimationSystem );

	// Initialize SceneGraphSystem
	mSceneGraphSystem = std::make_unique<eage::ecs::SceneGraphSystem>( *mECSRegistry );

	// Initialize RenderSystem
	mRenderSystem = std::make_unique<eage::ecs::RenderSystem>( *mRenderer, *mScenePass, *mECSRegistry );

	// Initialize PhysicsSystem
	mPhysicsSystem = std::make_unique<eage::ecs::PhysicsSystem>( *mECSRegistry );
	mPhysicsSystem->Initialize( { 0.f, 0.f }, 100.f ); // No gravity in space

	// Initialize SceneController
	mSceneController = std::make_unique<SceneController>();
	mSceneController->Subscribe( this );
	mSceneController->AddScene( static_cast<int>( config::SceneID::MAIN_SCENE ),
		std::make_unique<MainScene>( EngineContext{
			*mECSRegistry,
			*mRenderSystem,
			*mPhysicsSystem,
			*mAudioSystem,
			*mAnimationSystem,
			*mEffectSystem,
			*mInputController } ) );
	mSceneController->ChangeScene( static_cast<int>( config::SceneID::MAIN_SCENE ) );

	// Initialize PerformanceTracker
	mPerformanceTracker = std::make_unique<eage::profiling::PerformanceTracker>( *mRenderer );
	mImGuiPass->AddOverlayCallback( [this]()
	{
		mPerformanceTracker->DrawDebugGUI();
	} );

	// Initialize HudRenderSystem (registers its own overlay callback)
	mImGuiHudRenderer = std::make_unique<eage::graphics::ImGuiHudRenderer>( *mImGuiPass );
	mHudRenderSystem = std::make_unique<eage::ecs::HudRenderSystem>( *mECSRegistry, *mImGuiHudRenderer, ui_scale );

	return true;
}