#include "ImGuiRenderPass.h"

#include <imgui/vendor/imgui.h>
#include <imgui/vendor/imgui_impl_vulkan.h>
#include <imgui/vendor/imgui_impl_sdl2.h>

#include <imgui/ImGUILifetime.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanContext.h>
#include <utility/Logger.h>

using namespace eage::imgui;
using namespace eage::graphics;
using namespace utility;

namespace
{
	vk::RenderingAttachmentInfo create_attachment_info( vk::ImageView view, std::optional<vk::ClearValue> clear, vk::ImageLayout layout )
	{
		vk::RenderingAttachmentInfo attachment_info{};
		attachment_info.imageView = view;
		attachment_info.imageLayout = layout;
		attachment_info.loadOp = clear.has_value() ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
		if( clear.has_value() )
		{
			attachment_info.clearValue = clear.value();
		}

		return attachment_info;
	}
}

ImGuiRenderPass::ImGuiRenderPass(
	VulkanContext& context,
	SDL_Window& window,
	vk::Format swapchain_format,
	uint32_t min_image_count,
	uint32_t max_image_count )
{
	// Initialize ImGui lifetime (context, backends, descriptor pool)
	mLifetime = std::make_unique<ImGUILifetime>( context );
	mLifetime->Init( window, min_image_count, max_image_count, swapchain_format );

	// Create a sampler for sampling the scene render image (tool preview panels)
	vk::SamplerCreateInfo sampler_info{};
	sampler_info.magFilter = vk::Filter::eNearest;
	sampler_info.minFilter = vk::Filter::eNearest;

	vk::Device device = *context.logical_device;
	mSceneSampler = device.createSamplerUnique( sampler_info );

	// Writes to swapchain as a color attachment; clears by default (AnimTool).
	// Game shell calls SetClearColor( nullopt ) to composite as an overlay.
	mDesc.color_target = nullptr;
	mDesc.depth_target = nullptr;
	mDesc.swapchain_access = SwapchainAccess::ColorAttachment;
	mDesc.clear_color = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
}

ImGuiRenderPass::~ImGuiRenderPass()
{
	if( mSceneDescriptorSet != VK_NULL_HANDLE )
	{
		ImGui_ImplVulkan_RemoveTexture( mSceneDescriptorSet );
		mSceneDescriptorSet = VK_NULL_HANDLE;
	}
}

void
ImGuiRenderPass::LoadFont( const char* path, float size_px, eage::ecs::HudFontSize slot )
{
	ImFontAtlas* atlas = ImGui::GetIO().Fonts;
	ImFont* font = nullptr;

	if( path )
	{
		font = atlas->AddFontFromFileTTF( path, size_px );
	}
	else
	{
		ImFontConfig config;
		config.SizePixels = size_px;
		font = atlas->AddFontDefault( &config );
	}

	mFonts[static_cast<size_t>( slot )] = font;
}

void
ImGuiRenderPass::InitFontTexture( std::function<void( std::function<void( vk::CommandBuffer& )> )> immediate_submit )
{
	immediate_submit( []( vk::CommandBuffer& )
	{
		if( !ImGui_ImplVulkan_CreateFontsTexture() )
		{
			LOG_ERROR( "Failed to create IMGUI fonts texture." );
		}
	} );
}

ImFont*
ImGuiRenderPass::GetFont( eage::ecs::HudFontSize slot ) const
{
	ImFont* font = mFonts[static_cast<size_t>( slot )];
	return font ? font : ImGui::GetFont();
}

const RenderPassDesc&
ImGuiRenderPass::GetDesc() const
{
	return mDesc;
}

void
ImGuiRenderPass::Prepare( size_t frame_index )
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// Tool / debug UI overlays
	for( auto& fn : mOverlayCallbacks )
	{
		fn();
	}

	ImGui::Render();
}

void
ImGuiRenderPass::Execute( CommandBuffer& buffer, const FrameContext& ctx )
{
	vk::CommandBuffer cmd( static_cast<VkCommandBuffer>( buffer.GetNativeHandle() ) );
	vk::ImageView swapchain_view( static_cast<VkImageView>( ctx.swapchain_image_view_handle ) );
	vk::Extent2D extent{ ctx.swapchain_width, ctx.swapchain_height };

	std::optional<vk::ClearValue> clear_value;
	if( mDesc.clear_color.has_value() )
	{
		auto c = mDesc.clear_color.value();
		clear_value = vk::ClearColorValue{ std::array<float, 4>{ c.r, c.g, c.b, c.a } };
	}

	vk::RenderingAttachmentInfo color_attachment_info = create_attachment_info(
		swapchain_view, clear_value, vk::ImageLayout::eColorAttachmentOptimal );

	vk::RenderingInfo render_info{};
	render_info.colorAttachmentCount = 1;
	render_info.pColorAttachments = &color_attachment_info;
	render_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, extent };
	render_info.layerCount = 1;

	cmd.beginRendering( render_info );
	ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
	cmd.endRendering();
}

void
ImGuiRenderPass::AddOverlayCallback( std::function<void()> callback )
{
	mOverlayCallbacks.push_back( std::move( callback ) );
}

void
ImGuiRenderPass::SetClearColor( std::optional<glm::vec4> color )
{
	mDesc.clear_color = color;
}

void
ImGuiRenderPass::SetSceneInput( ManagedImage* image )
{
	if( mSceneDescriptorSet != VK_NULL_HANDLE )
	{
		ImGui_ImplVulkan_RemoveTexture( mSceneDescriptorSet );
		mSceneDescriptorSet = VK_NULL_HANDLE;
	}

	mSceneColorTarget = image;
	mDesc.input_images.clear();

	if( mSceneColorTarget != nullptr )
	{
		mSceneDescriptorSet = ImGui_ImplVulkan_AddTexture(
			*mSceneSampler,
			*mSceneColorTarget->image_view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		mDesc.input_images.push_back( PassInput{ mSceneColorTarget, ImageAccess::ShaderRead } );
	}
}

void*
ImGuiRenderPass::GetSceneTextureId() const
{
	return reinterpret_cast<void*>( mSceneDescriptorSet );
}

void
ImGuiRenderPass::ProcessEvent( const SDL_Event& event )
{
	ImGui_ImplSDL2_ProcessEvent( &event );
}
