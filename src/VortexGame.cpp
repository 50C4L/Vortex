#include "VortexGame.h"

#include <iostream>

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/SceneRenderPass.h>
#include <graphics/ImGuiRenderPass.h>
#include <graphics/ManagedVulkanResources.h>
#include <events/InputController.h>
#include <audio/AudioMixer.h>
#include <ecs/ECS.h>
#include <ecs/systems/AudioSystem.h>
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

VortexGame::~VortexGame()
{
	// GPU resources must be released before the renderer (which owns VMA/device).
	// Systems like RenderSystem hold VMA-allocated buffers and images, so they
	// must be torn down while the allocator is still alive.
	mRenderer->WaitForIdle();

	mPerformanceTracker.reset();
	mHudRenderSystem.reset();
	mRenderSystem.reset();
	mSceneGraphSystem.reset();
	mPhysicsSystem.reset();
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
	bool quit = false;
	while( !quit )
	{
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

		// @todo: delta time should be calulated here and pass down
		mSceneController->Update();

		mPhysicsSystem->Update();
		mAudioSystem->Update( 0.f );
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

	// Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		return false;
	}

	mWindow = utility::make_resource( 
		SDL_CreateWindow, SDL_DestroyWindow,
		"Vortex Game",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		static_cast<int>( config::DesignResolution::WIDTH ), static_cast<int>( config::DesignResolution::HEIGHT ),
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
		*mRenderer, *mRenderer->GetRenderImage(), *mRenderer->GetDepthImage() );
	mImGuiPass = std::make_unique<eage::graphics::ImGuiRenderPass>(
		mRenderer->GetVulkanContext(),
		*mWindow,
		mRenderer->GetSwapchainFormat(),
		eage::graphics::Renderer::MAX_FRAMES_IN_FLIGHT,
		mRenderer->GetSwapchainImageCount(),
		*mRenderer->GetRenderImage() );
	mImGuiPass->LoadFont( nullptr, 13.0f, eage::ecs::HudFontSize::SMALL );
	mImGuiPass->LoadFont( nullptr, 24.0f, eage::ecs::HudFontSize::MEDIUM );
	mImGuiPass->LoadFont( nullptr, 36.0f, eage::ecs::HudFontSize::LARGE );
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
	std::unordered_map<SDL_Keycode, uint64_t> keycode_to_event = {
		{ SDLK_a, static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ) },
		{ SDLK_d, static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ) },
		{ SDLK_w, static_cast<uint64_t>( GameEvents::PLAYER_THRUST ) },
		{ SDLK_j, static_cast<uint64_t>( GameEvents::PLAYER_SHOOT ) },
	};
	mInputController = std::make_unique<events::InputController>( std::move( keycode_to_event ) );

	// Initialize ECSRegistry
	mECSRegistry = std::make_unique<eage::ecs::ECSRegistry>();

	// Initialize AudioSystem
	mAudioSystem = std::make_unique<eage::ecs::AudioSystem>( *mECSRegistry, *mAudioMixer );

	// Initialize SceneGraphSystem
	mSceneGraphSystem = std::make_unique<eage::ecs::SceneGraphSystem>( *mECSRegistry );

	// Initialize RenderSystem
	mRenderSystem = std::make_unique<eage::ecs::RenderSystem>( *mRenderer, *mScenePass, *mECSRegistry );

	// Initialize PhysicsSystem
	mPhysicsSystem = std::make_unique<eage::ecs::PhysicsSystem>( *mECSRegistry );
	mPhysicsSystem->Initialize( { 0.f, 0.f }, 100.f ); // No gravity in space

	// Initialize SceneController
	mSceneController = std::make_unique<SceneController>();
	mSceneController->AddScene( static_cast<int>( config::SceneID::MAIN_SCENE ), 
		std::make_unique<MainScene>( *mInputController, *mECSRegistry, *mAudioSystem, *mRenderSystem, *mPhysicsSystem ) );
	mSceneController->ChangeScene( static_cast<int>( config::SceneID::MAIN_SCENE ) );
	mSceneGraphSystem->SetSceneRoot( mSceneController->GetCurrentSceneRoot() );

	// Initialize PerformanceTracker
	mPerformanceTracker = std::make_unique<eage::profiling::PerformanceTracker>( *mRenderer );
	mImGuiPass->AddOverlayCallback( [this]()
	{
		mPerformanceTracker->DrawDebugGUI();
	} );

	// Initialize HudRenderSystem (registers its own overlay callback)
	mHudRenderSystem = std::make_unique<eage::ecs::HudRenderSystem>( *mECSRegistry, *mImGuiPass );

	return true;
}