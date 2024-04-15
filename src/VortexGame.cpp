#include "VortexGame.h"

#include <iostream>

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <imgui/imgui_impl_sdl2.h>

#include "SceneController.h"
#include "game/MainScene.h"
#include "game/GameConfig.h"

using namespace vortex;

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
		}

		mSceneController->Update();

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
	mRenderer = std::make_unique<graphics::Renderer>( *mWindow );
	if( !mRenderer->Init() )
	{
		std::cerr << "Failed to initialize Renderer" << std::endl;
		return false;
	}

	// Initialize SceneController
	mSceneController = std::make_unique<SceneController>();
	mSceneController->AddScene( static_cast<int>( config::SceneID::MAIN_SCENE ), std::make_unique<MainScene>( *mRenderer ) );
	mSceneController->ChangeScene( static_cast<int>( config::SceneID::MAIN_SCENE ) );

	return true;
}