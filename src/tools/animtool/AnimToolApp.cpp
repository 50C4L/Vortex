#include "AnimToolApp.h"

#include <functional>
#include <iostream>
#include <thread>

#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <utility/Filesystem.h>
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
	constexpr float WORKSPACE_HEIGHT_RATIO = 0.30f;
	constexpr float PREVIEW_WIDTH_RATIO = 0.50f;

	struct ToolLayout
	{
		ImVec2 workspace_pos;
		ImVec2 workspace_size;
		ImVec2 preview_pos;
		ImVec2 preview_size;
		ImVec2 editor_pos;
		ImVec2 editor_size;
	};

	ToolLayout compute_tool_layout()
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const float menu_bar_height = ImGui::GetFrameHeight();
		const float work_height = viewport->Size.y - menu_bar_height;
		const float workspace_height = work_height * WORKSPACE_HEIGHT_RATIO;
		const float remaining_height = work_height - workspace_height;
		const float half_width = viewport->Size.x * PREVIEW_WIDTH_RATIO;
		const float remaining_y = viewport->Pos.y + menu_bar_height + workspace_height;

		return {
			{ viewport->Pos.x, viewport->Pos.y + menu_bar_height },
			{ viewport->Size.x, workspace_height },
			{ viewport->Pos.x, remaining_y },
			{ half_width, remaining_height },
			{ viewport->Pos.x + half_width, remaining_y },
			{ viewport->Size.x - half_width, remaining_height },
		};
	}

	void begin_fixed_panel( const char* title, ImVec2 pos, ImVec2 size )
	{
		ImGui::SetNextWindowPos( pos, ImGuiCond_Always );
		ImGui::SetNextWindowSize( size, ImGuiCond_Always );

		constexpr ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoMove                |
			ImGuiWindowFlags_NoResize              |
			ImGuiWindowFlags_NoCollapse            |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoSavedSettings;

		ImGui::Begin( title, nullptr, flags );
	}

	void draw_main_menu_bar()
	{
		if( !ImGui::BeginMainMenuBar() )
		{
			return;
		}

		if( ImGui::BeginMenu( "File" ) )
		{
			if( ImGui::MenuItem( "Open Project" ) )
			{
				// TODO: open project
			}

			if( ImGui::MenuItem( "Save Project" ) )
			{
				// TODO: save project
			}

			if( ImGui::MenuItem( "Load Images" ) )
			{
				// TODO: load images
			}

			if( ImGui::MenuItem( "Export" ) )
			{
				// TODO: export
			}

			ImGui::Separator();

			if( ImGui::MenuItem( "Exit" ) )
			{
				// TODO: exit application
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	void draw_work_space()
	{
		const ToolLayout layout = compute_tool_layout();
		begin_fixed_panel( "Work Space", layout.workspace_pos, layout.workspace_size );
		// TODO: frame thumbnail strip
		ImGui::Text( "Animation sequence" );
		ImGui::End();
	}

	void draw_preview( eage::graphics::ImGuiRenderPass& imgui_pass )
	{
		const ToolLayout layout = compute_tool_layout();
		begin_fixed_panel( "Preview", layout.preview_pos, layout.preview_size );

		const ImVec2 avail = ImGui::GetContentRegionAvail();
		if( avail.x > 0.f && avail.y > 0.f )
		{
			ImGui::Image( reinterpret_cast<ImTextureID>( imgui_pass.GetSceneTextureId() ), avail );
		}

		ImGui::End();
	}

	void draw_editor_panel()
	{
		const ToolLayout layout = compute_tool_layout();
		begin_fixed_panel( "Editor Panel", layout.editor_pos, layout.editor_size );
		// TODO: animation editor controls
		ImGui::Text( "Editor Panel" );
		ImGui::End();
	}
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
	utility::init_content_working_directory();

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

	mImGuiPass->SetSceneViewportTopRatio( WORKSPACE_HEIGHT_RATIO );

	mImGuiPass->AddOverlayCallback( [this]()
	{
		draw_work_space();
		draw_preview( *mImGuiPass );
		draw_editor_panel();
		draw_main_menu_bar();
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
