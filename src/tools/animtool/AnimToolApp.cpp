#include "AnimToolApp.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <cctype>
#include <thread>

#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <graphics/Camera.h>
#include <graphics/MaterialBuilder.h>

#include <utility/Pointers.h>
#include <utility/Logger.h>
#include <utility/Filesystem.h>
#include <graphics/Renderer.h>
#include <graphics/SceneRenderPass.h>
#include <graphics/ImGuiRenderPass.h>

#include "FileDialog.h"
#include "AnimationExporter.h"
#include "FrameSequence.h"
#include "FrameThumbnail.h"
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
	constexpr float THUMBNAIL_MAX_HEIGHT = 96.f;
	constexpr float WORKSPACE_STRIP_VERTICAL_PADDING = 12.f;
	constexpr ImVec2 FRAME_IMAGE_UV_MIN( 0.f, 1.f );
	constexpr ImVec2 FRAME_IMAGE_UV_MAX( 1.f, 0.f );
	constexpr ImVec2 SCENE_IMAGE_UV_MIN( 0.f, 0.f );
	constexpr ImVec2 SCENE_IMAGE_UV_MAX( 1.f, 1.f );

	void draw_export_modal(
		ExportDialogState& export_state,
		FileDialog& file_dialog,
		const FrameSequence& frame_sequence )
	{
		if( export_state.show_modal )
		{
			ImGui::OpenPopup( "Export Animation" );
			export_state.show_modal = false;
		}

		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "Export Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
		{
			ImGui::TextUnformatted( "Animation directory name:" );
			ImGui::InputText( "##export_name", export_state.animation_name, sizeof( export_state.animation_name ) );

			if( ImGui::Button( "Choose Location and Export", ImVec2( 240.f, 0.f ) ) )
			{
				if( frame_sequence.GetFrameCount() == 0 )
				{
					utility::LOG_ERROR() << "Cannot export an empty frame sequence.";
				}
				else if( auto parent_folder = file_dialog.GetFolderPath() )
				{
					const std::filesystem::path output_directory =
						std::filesystem::path( *parent_folder ) / export_state.animation_name;

					if( AnimationExporter::Export( frame_sequence, output_directory ) )
					{
						utility::LOG() << "Exported animation to: " << output_directory.string();
						ImGui::CloseCurrentPopup();
					}
				}
			}

			ImGui::SameLine();

			if( ImGui::Button( "Cancel", ImVec2( 120.f, 0.f ) ) )
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

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

	void draw_main_menu_bar(
		FileDialog& file_dialog,
		FrameSequence& frame_sequence,
		eage::graphics::Renderer& renderer,
		ExportDialogState& export_state )
	{
		if( !ImGui::BeginMainMenuBar() )
		{
			return;
		}

		if( ImGui::BeginMenu( "File" ) )
		{
			if( ImGui::MenuItem( "Open Project" ) )
			{
				if( auto path = file_dialog.GetFilePath( { "json" } ) )
				{
					utility::LOG() << "Open Project: " << *path;
				}
			}

			if( ImGui::MenuItem( "Save Project" ) )
			{
				// TODO: save project
			}

			if( ImGui::MenuItem( "Load PNG Folder..." ) )
			{
				if( auto path = file_dialog.GetFolderPath() )
				{
					const size_t added_count = frame_sequence.AppendPngFolder( *path, renderer );
					utility::LOG() << "Added " << added_count << " frame(s) from: " << *path;
				}
			}

			if( ImGui::MenuItem( "Load GIF..." ) )
			{
				if( auto path = file_dialog.GetFilePath( { "gif" } ) )
				{
					const size_t added_count = frame_sequence.AppendGif( *path, renderer );
					utility::LOG() << "Added " << added_count << " frame(s) from GIF: " << *path;
				}
			}

			if( ImGui::MenuItem( "Export..." ) )
			{
				export_state.show_modal = true;
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

	void draw_frame_thumbnail_strip( FrameSequence& frame_sequence )
	{
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		if( avail.x <= 0.f || avail.y <= 0.f )
		{
			return;
		}

		const float frame_count_height = ImGui::GetTextLineHeightWithSpacing();
		const float strip_height = std::min(
			THUMBNAIL_MAX_HEIGHT,
			avail.y - ( WORKSPACE_STRIP_VERTICAL_PADDING * 2.f ) - frame_count_height );

		const float content_height = strip_height + frame_count_height;
		const float top_spacer = std::max( WORKSPACE_STRIP_VERTICAL_PADDING, ( avail.y - content_height ) * 0.5f );

		ImGui::Dummy( ImVec2( 0.f, top_spacer ) );

		ImGui::BeginChild(
			"##frame_strip",
			ImVec2( avail.x, strip_height ),
			ImGuiChildFlags_None,
			ImGuiWindowFlags_HorizontalScrollbar );

		const float thumb_height = ImGui::GetContentRegionAvail().y;

		for( size_t i = 0; i < frame_sequence.GetFrameCount(); ++i )
		{
			const FrameThumbnail& frame = frame_sequence.GetFrame( i );
			const float aspect = static_cast<float>( frame.GetWidth() ) / static_cast<float>( frame.GetHeight() );
			const ImVec2 thumb_size( thumb_height * aspect, thumb_height );

			ImGui::PushID( static_cast<int>( i ) );

			const bool is_selected = frame_sequence.GetSelectedFrame().has_value()
				&& frame_sequence.GetSelectedFrame().value() == i;

			if( is_selected )
			{
				ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.00f, 0.83f, 1.00f, 1.00f ) );
				ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 2.f );
			}

			if( ImGui::ImageButton( "##thumb", frame.GetTextureId(), thumb_size, FRAME_IMAGE_UV_MIN, FRAME_IMAGE_UV_MAX ) )
			{
				frame_sequence.SetSelectedFrame( i );
			}

			if( is_selected )
			{
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
			}

			ImGui::PopID();

			if( i + 1 < frame_sequence.GetFrameCount() )
			{
				ImGui::SameLine();
			}
		}

		ImGui::EndChild();

		ImGui::Text( "%zu frame(s)", frame_sequence.GetFrameCount() );
	}

	void draw_work_space( FrameSequence& frame_sequence )
	{
		const ToolLayout layout = compute_tool_layout();
		begin_fixed_panel( "Work Space", layout.workspace_pos, layout.workspace_size );
		draw_frame_thumbnail_strip( frame_sequence );
		ImGui::End();
	}

	void draw_preview(
		const FrameSequence& frame_sequence,
		void* scene_texture_id,
		PreviewPlaybackState& playback,
		bool& play_requested )
	{
		const ToolLayout layout = compute_tool_layout();
		begin_fixed_panel( "Preview", layout.preview_pos, layout.preview_size );

		const bool can_play = frame_sequence.GetFrameCount() > 0;
		ImGui::BeginDisabled( !can_play );
		if( ImGui::Button( "Play" ) )
		{
			play_requested = true;
		}
		ImGui::EndDisabled();

		const ImVec2 avail = ImGui::GetContentRegionAvail();
		if( avail.x <= 0.f || avail.y <= 0.f )
		{
			ImGui::End();
			return;
		}

		if( !can_play )
		{
			ImGui::TextUnformatted( "No frames loaded" );
			ImGui::End();
			return;
		}

		const size_t display_frame = playback.playing
			? playback.current_frame
			: frame_sequence.GetSelectedFrame().value_or( 0 );

		const FrameThumbnail& frame = frame_sequence.GetFrame( display_frame );
		const float aspect = static_cast<float>( frame.GetWidth() ) / static_cast<float>( frame.GetHeight() );
		const float avail_aspect = avail.x / avail.y;

		ImVec2 display_size = avail;
		if( aspect > avail_aspect )
		{
			display_size.y = avail.x / aspect;
		}
		else
		{
			display_size.x = avail.y * aspect;
		}

		const ImVec2 cursor_pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos( ImVec2(
			cursor_pos.x + ( avail.x - display_size.x ) * 0.5f,
			cursor_pos.y + ( avail.y - display_size.y ) * 0.5f ) );

		ImGui::Image(
			reinterpret_cast<ImTextureID>( scene_texture_id ),
			display_size,
			SCENE_IMAGE_UV_MIN,
			SCENE_IMAGE_UV_MAX );

		ImGui::End();
	}

	void draw_editor_panel( FrameSequence& frame_sequence )
	{
		const ToolLayout layout = compute_tool_layout();
		begin_fixed_panel( "Editor Panel", layout.editor_pos, layout.editor_size );

		if( frame_sequence.GetFrameCount() == 0 )
		{
			ImGui::TextUnformatted( "No frames loaded" );
			ImGui::End();
			return;
		}

		int& animation_duration_ms = frame_sequence.GetAnimationDurationMs();
		ImGui::InputInt( "Duration (ms)", &animation_duration_ms, 0, 0 );
		if( ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit() )
		{
			frame_sequence.SetAnimationDurationMs( animation_duration_ms );
		}

		const float per_frame_ms = frame_sequence.GetFrameDurationSec() * 1000.f;
		ImGui::Text( "Per frame: %.1f ms", per_frame_ms );

		const std::optional<size_t> selected_frame = frame_sequence.GetSelectedFrame();
		if( !selected_frame.has_value() )
		{
			ImGui::TextUnformatted( "Select a frame to inspect" );
			ImGui::End();
			return;
		}

		const FrameThumbnail& frame = frame_sequence.GetFrame( selected_frame.value() );
		ImGui::Text( "Frame: %zu", selected_frame.value() + 1 );
		ImGui::Text( "Size: %d x %d", frame.GetWidth(), frame.GetHeight() );
		ImGui::Text( "Bindless index: %u", frame.GetBindlessTextureIndex() );
		ImGui::Text( "Source: %s", frame.GetSourcePath().filename().string().c_str() );

		const std::string source_extension = frame.GetSourcePath().extension().string();
		if( source_extension.size() == 4
			&& std::tolower( static_cast<unsigned char>( source_extension[1] ) ) == 'g'
			&& std::tolower( static_cast<unsigned char>( source_extension[2] ) ) == 'i'
			&& std::tolower( static_cast<unsigned char>( source_extension[3] ) ) == 'f' )
		{
			ImGui::Text( "GIF frame index: %d", frame.GetFrameIndexInSource() );
		}

		ImGui::End();
	}
}

void
PreviewPlaybackState::Start()
{
	playing = true;
	current_frame = 0;
	elapsed_sec = 0.f;
}

void
PreviewPlaybackState::Stop()
{
	playing = false;
	elapsed_sec = 0.f;
}

void
PreviewPlaybackState::Update( float delta_time_sec, const FrameSequence& frame_sequence )
{
	if( !playing || frame_sequence.GetFrameCount() == 0 )
	{
		return;
	}

	elapsed_sec += delta_time_sec;

	while( playing )
	{
		const float frame_duration_sec = frame_sequence.GetFrameDurationSec();

		if( elapsed_sec < frame_duration_sec )
		{
			break;
		}

		elapsed_sec -= frame_duration_sec;
		++current_frame;

		if( current_frame >= frame_sequence.GetFrameCount() )
		{
			Stop();
			break;
		}
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

	if( mFrameSequence )
	{
		mFrameSequence->Clear();
	}

	mImGuiPass.reset();
	mScenePass.reset();
	mRenderer.reset();
	mFrameSequence.reset();
	mRenderSystem.reset();
	mECSRegistry.reset();
	mPreviewCamera.reset();
	mFileDialog.reset();
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

	mFileDialog = std::make_unique<FileDialog>( mWindow.get() );
	mFrameSequence = std::make_unique<FrameSequence>();

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
		DrawToolUI();
	} );

	mRenderer->AddRenderPass( mScenePass.get() );
	mRenderer->AddRenderPass( mImGuiPass.get() );

	InitPreviewRendering();

	return true;
}

void
AnimToolApp::InitPreviewRendering()
{
	mECSRegistry = std::make_unique<eage::ecs::ECSRegistry>();
	mRenderSystem = std::make_unique<eage::ecs::RenderSystem>( *mRenderer, *mScenePass, *mECSRegistry );

	mPreviewCamera = std::make_unique<eage::graphics::OrthographicCamera>(
		-0.5f, 0.5f, -0.5f, 0.5f, 0.1f, 100.0f );
	mPreviewCamera->SetPosition( { 0.f, 0.f, 2.f } );
	mPreviewVirtualResolution = { 1.f, 1.f };

	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.SetAlphaBlending( true )
		.EnableDepthTest( true )
		.Build();

	mPreviewMaterialId = mRenderSystem->CreateMaterial( material_property );

	mPreviewEntity = mECSRegistry->CreateEntity();
	mECSRegistry->AddComponent( mPreviewEntity, eage::ecs::TransformComponent{} );
	mPreviewMeshId = mRenderSystem->CreateSpriteMesh( 1.f, 1.f );
	mRenderSystem->AttachRenderable( mPreviewEntity, mPreviewMeshId, mPreviewMaterialId, 0, false );
}

void
AnimToolApp::DrawToolUI()
{
	draw_work_space( *mFrameSequence );
	draw_editor_panel( *mFrameSequence );
	draw_preview( *mFrameSequence, mImGuiPass->GetSceneTextureId(), mPreviewPlayback, mPlayRequested );
	draw_main_menu_bar( *mFileDialog, *mFrameSequence, *mRenderer, mExportState );
	draw_export_modal( mExportState, *mFileDialog, *mFrameSequence );

	if( mPlayRequested )
	{
		mFrameSequence->SetAnimationDurationMs( mFrameSequence->GetAnimationDurationMs() );
	}
}

void
AnimToolApp::UpdatePreviewPlayback( float delta_time_sec )
{
	mPreviewPlayback.Update( delta_time_sec, *mFrameSequence );
}

void
AnimToolApp::UpdatePreviewSprite()
{
	if( mPreviewEntity == 0 || !mRenderSystem )
	{
		return;
	}

	auto& render_cmp = mECSRegistry->GetComponent<eage::ecs::RenderComponent>( mPreviewEntity );

	size_t display_frame_index = 0;
	if( mPreviewPlayback.playing )
	{
		display_frame_index = mPreviewPlayback.current_frame;
	}
	else if( const std::optional<size_t> selected_frame = mFrameSequence->GetSelectedFrame();
		selected_frame.has_value() )
	{
		display_frame_index = selected_frame.value();
	}
	else if( mFrameSequence->GetFrameCount() == 0 )
	{
		render_cmp.visible = false;
		return;
	}

	const FrameThumbnail& frame = mFrameSequence->GetFrame( display_frame_index );
	render_cmp.texture_index = frame.GetBindlessTextureIndex();
	render_cmp.visible = true;

	const bool size_changed = frame.GetWidth() != mPreviewMeshWidth
		|| frame.GetHeight() != mPreviewMeshHeight;

	if( size_changed )
	{
		mPreviewMeshWidth = frame.GetWidth();
		mPreviewMeshHeight = frame.GetHeight();
		mPreviewMeshId = mRenderSystem->CreateSpriteMesh(
			static_cast<float>( mPreviewMeshWidth ),
			static_cast<float>( mPreviewMeshHeight ) );
		render_cmp.mesh_buffer_id = mPreviewMeshId;

		const float half_width = static_cast<float>( mPreviewMeshWidth ) * 0.5f;
		const float half_height = static_cast<float>( mPreviewMeshHeight ) * 0.5f;
		mPreviewCamera = std::make_unique<eage::graphics::OrthographicCamera>(
			-half_width, half_width, -half_height, half_height, 0.1f, 100.0f );
		mPreviewCamera->SetPosition( { 0.f, 0.f, 2.f } );
		mPreviewVirtualResolution = glm::vec2(
			static_cast<float>( mPreviewMeshWidth ),
			static_cast<float>( mPreviewMeshHeight ) );
	}

}

void
AnimToolApp::Run()
{
	bool quit = false;
	auto last_frame_time = std::chrono::steady_clock::now();

	while( !quit )
	{
		const auto frame_time = std::chrono::steady_clock::now();
		const float delta_time_sec = std::chrono::duration<float>( frame_time - last_frame_time ).count();
		last_frame_time = frame_time;

		SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
			{
				quit = true;
			}

			mImGuiPass->ProcessEvent( event );
		}

		UpdatePreviewPlayback( delta_time_sec );
		UpdatePreviewSprite();

		if( mRenderSystem && mPreviewCamera )
		{
			mRenderSystem->SetCamera( *mPreviewCamera, mPreviewVirtualResolution );
			mRenderSystem->Update();
		}

		mRenderer->Render();

		if( mPlayRequested )
		{
			mPreviewPlayback.Start();
			mPlayRequested = false;
		}
	}

	mRenderer->WaitForIdle();
}
