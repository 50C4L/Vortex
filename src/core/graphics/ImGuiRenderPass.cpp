#include "ImGuiRenderPass.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_sdl2.h>

#include <graphics/ImGUILifetime.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanContext.h>
#include <utility/Logger.h>

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
	uint32_t max_image_count,
	ManagedImage& scene_color_target )
	: mSceneColorTarget( scene_color_target )
{
	// Initialize ImGui lifetime (context, backends, descriptor pool)
	mLifetime = std::make_unique<ImGUILifetime>( context );
	mLifetime->Init( window, min_image_count, max_image_count, swapchain_format );

	// Create a sampler for sampling the scene render image
	vk::SamplerCreateInfo sampler_info{};
	sampler_info.magFilter = vk::Filter::eNearest;
	sampler_info.minFilter = vk::Filter::eNearest;

	vk::Device device = *context.logical_device;
	mSceneSampler = device.createSamplerUnique( sampler_info );

	// Register the scene color target with ImGui as a texture
	mSceneDescriptorSet = ImGui_ImplVulkan_AddTexture(
		*mSceneSampler,
		*mSceneColorTarget.image_view,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	// Build the pass descriptor -- writes to swapchain (nullptr), reads scene image
	mDesc.color_target = nullptr;
	mDesc.depth_target = nullptr;
	mDesc.input_images.push_back( &mSceneColorTarget );
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

	// Fullscreen borderless window displaying the scene texture
	const ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos( vp->Pos );
	ImGui::SetNextWindowSize( vp->Size );

	constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration          |
		ImGuiWindowFlags_NoMove                |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoSavedSettings       |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
	if( ImGui::Begin( "GameViewport", nullptr, flags ) )
	{
		ImGui::Image( (ImTextureID)mSceneDescriptorSet, vp->Size );
	}
	ImGui::End();
	ImGui::PopStyleVar();

	// Debug overlay components
	for( auto& fn : mOverlayCallbacks )
	{
		fn();
	}

	ImGui::Render();
}

void
ImGuiRenderPass::Execute( vk::CommandBuffer& cmd, const ExecutionContext& ctx )
{
	vk::ClearValue clear_value;
	clear_value.color = vk::ClearColorValue{ std::array<float,4>{ 0.f, 0.f, 0.f, 1.f } };
	vk::RenderingAttachmentInfo color_attachment_info = create_attachment_info(
		ctx.swapchain_image_view, clear_value, vk::ImageLayout::eColorAttachmentOptimal );

	vk::RenderingInfo render_info{};
	render_info.colorAttachmentCount = 1;
	render_info.pColorAttachments = &color_attachment_info;
	render_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, ctx.swapchain_extent };
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
ImGuiRenderPass::ProcessEvent( const SDL_Event& event )
{
	ImGui_ImplSDL2_ProcessEvent( &event );
}
