#include "AnimToolApp.h"

#include <functional>
#include <iostream>
#include <thread>

#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/SceneRenderPass.h>
#include <graphics/ImGuiRenderPass.h>

#include "ToolUIStyle.h"

using namespace animtool;

namespace
{
	constexpr int WINDOW_WIDTH = 1920;
	constexpr int WINDOW_HEIGHT = 1080;
	constexpr uint32_t SCENE_WIDTH = 1920;
	constexpr uint32_t SCENE_HEIGHT = 1080;
}

AnimToolApp::AnimToolApp()
{
}

AnimToolApp::~AnimToolApp()
{
	if( mRenderer )
	{
		mRenderer->WaitForIdle();
	}

	mImGuiPass.reset();
	mScenePass.reset();
	mRenderer.reset();
	mWindow.reset();
	SDL_Quit();
}

bool
AnimToolApp::Init()
{
	utility::GetLogger().RegisterThread( std::this_thread::get_id(), "Main" );

	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		return false;
	}

	mWindow = utility::make_resource(
		SDL_CreateWindow, SDL_DestroyWindow,
		"AnimTool",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );
	if( !mWindow )
	{
		std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
		return false;
	}

	mRenderer = std::make_unique<eage::graphics::Renderer>( *mWindow );
	if( !mRenderer->Init() )
	{
		std::cerr << "Failed to initialize Renderer" << std::endl;
		return false;
	}

	mScenePass = std::make_unique<eage::graphics::SceneRenderPass>(
		*mRenderer, SCENE_WIDTH, SCENE_HEIGHT );
	mImGuiPass = std::make_unique<eage::graphics::ImGuiRenderPass>(
		mRenderer->GetVulkanContext(),
		*mWindow,
		mRenderer->GetSwapchainFormat(),
		eage::graphics::Renderer::MAX_FRAMES_IN_FLIGHT,
		mRenderer->GetSwapchainImageCount(),
		mScenePass->GetColorTarget() );
	mImGuiPass->LoadFont( nullptr, 16.0f, eage::ecs::HudFontSize::MEDIUM );
	mImGuiPass->InitFontTexture( [this]( std::function<void( vk::CommandBuffer& )> work )
	{
		mRenderer->ImmediateSubmit( std::move( work ) );
	} );
	apply_tool_style();

	mImGuiPass->AddOverlayCallback( []()
	{
		ImGui::Begin( "Frame Animation Tool" );
		ImGui::Text( "Placeholder - animation editor UI coming soon." );
		ImGui::End();
	} );

	mRenderer->AddRenderPass( mScenePass.get() );
	mRenderer->AddRenderPass( mImGuiPass.get() );

	return true;
}

void
AnimToolApp::Run()
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
		}

		mRenderer->Render();
	}

	mRenderer->WaitForIdle();
}
