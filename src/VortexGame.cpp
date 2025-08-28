#include "VortexGame.h"

#include <iostream>

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <events/InputController.h>
#include <imgui/imgui_impl_sdl2.h>
#include <audio/AudioMixer.h>
#include <ecs/ECS.h>
#include <ecs/systems/RenderSystem.h>

#include "SceneController.h"
#include "game/MainScene.h"
#include "game/GameConfig.h"

using namespace vortex;
using namespace vortex::config;

VortexGame::VortexGame()
{
}

VortexGame::~VortexGame()
{
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

			ImGui_ImplSDL2_ProcessEvent( &event );

			if( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
			{
				mInputController->Handle( event );
			}
		}

		mSceneController->Update();

		mRenderSystem->PrepareRenderInfo();
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

	// Initialize AudioMixer
	mAudioMixer = std::make_unique<audio::AudioMixer>();

	// Initialize InputController
	std::unordered_map<SDL_Keycode, uint64_t> keycode_to_event = {
		{ SDLK_a, static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ) },
		{ SDLK_d, static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ) },
		{ SDLK_w, static_cast<uint64_t>( GameEvents::PLAYER_THRUST ) }
	};
	mInputController = std::make_unique<events::InputController>( std::move( keycode_to_event ) );

	// Initialize ECSRegistry
	mECSRegistry = std::make_unique<eage::ecs::ECSRegistry>();

	// Initialize RenderSystem
	mRenderSystem = std::make_unique<eage::ecs::RenderSystem>( *mRenderer, *mECSRegistry );

	// Initialize SceneController
	mSceneController = std::make_unique<SceneController>();
	mSceneController->AddScene( static_cast<int>( config::SceneID::MAIN_SCENE ), 
		std::make_unique<MainScene>( *mRenderer, *mInputController, *mAudioMixer, *mECSRegistry, *mRenderSystem ) );
	mSceneController->ChangeScene( static_cast<int>( config::SceneID::MAIN_SCENE ) );

	return true;
}