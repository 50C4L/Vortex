#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_system.h>
#include <iostream>

#include <utility/Logger.h>
#include <graphics/ImageUtilities.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanContext.h>
#include <graphics/VulkanSwapChain.h>
#include <graphics/VulkanShader.h>
#include <graphics/VulkanCommandContext.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VMAWrapper.h>
#include <graphics/ImGUILifetime.h>

#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_sdl2.h>

using namespace graphics;
using namespace utility;

namespace
{
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2; // Double buffering

	vk::SubmitInfo2 create_submit_info( vk::CommandBufferSubmitInfo cmd_submit_info, std::optional<vk::SemaphoreSubmitInfo> semaphore_wait_info, std::optional<vk::SemaphoreSubmitInfo> semaphore_signal_info )
	{
		vk::SubmitInfo2 submit_info{};
		submit_info.waitSemaphoreInfoCount = semaphore_wait_info.has_value() ? 1 : 0;
		submit_info.pWaitSemaphoreInfos = semaphore_wait_info.has_value() ? &semaphore_wait_info.value() : nullptr;
		submit_info.signalSemaphoreInfoCount = semaphore_signal_info.has_value() ? 1 : 0;
		submit_info.pSignalSemaphoreInfos = semaphore_signal_info.has_value() ? &semaphore_signal_info.value() : nullptr;
		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &cmd_submit_info;

		return submit_info;
	}

	vk::RenderingAttachmentInfo create_attachment_info( vk::ImageView& view, std::optional<vk::ClearValue> clear, vk::ImageLayout layout )
	{
		vk::RenderingAttachmentInfo color_attachment_info{};
		color_attachment_info.imageView = view;
		color_attachment_info.imageLayout = layout;
		color_attachment_info.loadOp = clear.has_value() ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		color_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
		if( clear.has_value() )
		{
			color_attachment_info.clearValue = clear.value();
		}

		return color_attachment_info;
	}

	void render_imgui( vk::CommandBuffer& cmd, vk::ImageView& target_image_view, vk::Extent2D extent )
	{
		vk::RenderingAttachmentInfo color_attachment_info = create_attachment_info( target_image_view, std::nullopt, vk::ImageLayout::eGeneral );
		vk::RenderingInfo render_info{};
		render_info.colorAttachmentCount = 1;
		render_info.pColorAttachments = &color_attachment_info;
		render_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, std::move( extent ) };
		render_info.layerCount = 1;

		cmd.beginRendering( render_info );
		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
		cmd.endRendering();
	}
}


Renderer::Renderer()
	: mFrameNumber( 0 )
{
}

Renderer::~Renderer()
{
}

bool
Renderer::Init( SDL_Window& window )
{
	LOG( "Initializing vulkan context ..." );
	mContext = std::make_unique<VulkanContext>( window );

	int width, height = 0;
	SDL_Vulkan_GetDrawableSize( &window, &width, &height );

	LOG( "Initializing vulkan swap chain ..." );
	mSwapChain = std::make_unique<VulkanSwapChain>( *mContext, static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) );
	if( mSwapChain->GetImages().size() <MAX_FRAMES_IN_FLIGHT )
	{
		LOG_ERROR( "Swap chain does not have enough images." );
		return false;
	}

	LOG( "Initializing VMA ..." );
	mVMA = std::make_unique<VMAWrapper>( *mContext );

	LOG( "Initializing vulkan command buffers ..." );
	for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		mFrames.push_back( Frame{ {}, std::make_unique<VulkanCommandContext>( *mContext ) } );
	}
	mImmidiateCommandContext = std::make_unique<VulkanCommandContext>( *mContext );

	LOG( "Creating render image ..." );
	vk::ImageUsageFlags image_usage_flags;
	image_usage_flags |= vk::ImageUsageFlagBits::eTransferSrc;
	image_usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
	image_usage_flags |= vk::ImageUsageFlagBits::eStorage;
	image_usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;

	mRenderImage = std::make_unique<ManagedImage>(
		mContext->logical_device.get(),
		*mVMA->allocator.get(),
		vk::Extent3D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 },
		vk::Format::eR16G16B16A16Sfloat,
		image_usage_flags
	);

	// Init descriptor set layout
	InitDescriptors();

	// Init pipelines
	InitPipelines();

	// Init IMGUI
	InitImGUI( window );

	return true;
}

void
Renderer::Render()
{
	PrepareImGUI();

	auto& frame = GetCurrentFrame();
	auto& cmd = frame.command_context->GetPrimaryBuffer();
	uint32_t next_image_index = mSwapChain->GetNextImage( frame.command_context->GetSwapchainSemaphore() );

	frame.command_context->WaitForCompletion();
	frame.command_context->Reset();
	frame.command_context->Begin();

	// Transition the main render image to a general layout
	transition_image( cmd, mRenderImage->GetImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral );

	// Actual rendering here

	/* TODO
	for( const auto& renderable : GetCurrentFrame().mRenderables )
	{
		renderable.Render( *vulkan_main_buffer );
	}
	*/

	auto render_extent = mRenderImage->GetExtent2D();
	cmd.bindPipeline( vk::PipelineBindPoint::eCompute, mBackgroundPipeline.get() );
	cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, mBackgroundPipelineLayout.get(), 0, 1, &mRenderImageDescriptorSet.get(), 0, nullptr );
	cmd.dispatch( static_cast<uint32_t>( std::ceil( render_extent.width / 16.0 ) ), static_cast<uint32_t>( std::ceil( render_extent.height / 16.0 ) ), 1 );

	// End of rendering

	// Transition the main render image and the current swapchain image to the appropriate layout, so later we can copy the render image to the swapchain image
	transition_image( cmd, mRenderImage->GetImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal );
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal );

	// Copy the render image to the swapchain image
	copy_image_to_image( cmd, mRenderImage->GetImage(), mSwapChain->GetImages()[ next_image_index ], render_extent, mSwapChain->GetExtent() );

	// Transition the swapchain image to a color optimal layout so we can draw imgui on it
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal );

	// Render imgui
	render_imgui( cmd, mSwapChain->GetImageViews()[ next_image_index ].get(), mSwapChain->GetExtent() );

	// Transition the swapchain image back to the present layout
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR );

	frame.command_context->End();

	Submit();

	Present( next_image_index );

	mFrameNumber++;
}

void
Renderer::AddToRenderQueue( std::shared_ptr<Renderable> renderable )
{
	if( mFrames.empty() )
	{
		LOG_ERROR( "No frames available, Init() must be called first." );
		return;
	}
	GetCurrentFrame().renderables.push_back( renderable );
}

void
Renderer::WaitForIdle()
{
	mContext->logical_device->waitIdle();
}

Renderer::Frame&
Renderer::GetCurrentFrame()
{
	return mFrames[ mFrameNumber % MAX_FRAMES_IN_FLIGHT ];
}

void
Renderer::Submit()
{
	auto& frame = GetCurrentFrame();

	auto submit_info = create_submit_info( 
		frame.command_context->GetSubmitInfo(), 
		frame.command_context->GetSwapchainSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput ),
		frame.command_context->GetPresentSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
	);

	mContext->graphics_queue.submit2( submit_info, frame.command_context->GetFence() );
}

void
Renderer::Present( uint32_t image_index )
{
	auto& frame = GetCurrentFrame();

	vk::PresentInfoKHR present_info{};
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &frame.command_context->GetPresentSemaphore();
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &mSwapChain->GetSwapChain();
	present_info.pImageIndices      = &image_index;

	std::ignore = mContext->present_queue.presentKHR( present_info );
}

void
Renderer::InitDescriptors()
{
	LOG( "Initializing descriptor sets ..." );
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ vk::DescriptorType::eStorageImage, 1 }
	};
	mDescriptorAllocator = std::make_unique<DescriptorAllocator>( *mContext->logical_device, 10, sizes );

	{
		DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding( 0, vk::DescriptorType::eStorageImage );
		mRenderImageDescriptorSetLayout = layout_builder.Build( *mContext->logical_device, vk::ShaderStageFlagBits::eCompute );
	}

	mRenderImageDescriptorSet = mDescriptorAllocator->Allocate( *mRenderImageDescriptorSetLayout );

	vk::DescriptorImageInfo image_info;
	image_info.imageLayout = vk::ImageLayout::eGeneral;
	image_info.imageView = mRenderImage->GetImageView();

	vk::WriteDescriptorSet write;
	write.dstSet = mRenderImageDescriptorSet.get();
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = vk::DescriptorType::eStorageImage;
	write.pImageInfo = &image_info;

	mContext->logical_device->updateDescriptorSets( write, nullptr );
}

void
Renderer::InitPipelines()
{
	InitBackgroundPipeline();
}

bool
Renderer::InitBackgroundPipeline()
{
	vk::PipelineLayoutCreateInfo compute_layout_info{};
	compute_layout_info.pSetLayouts = &mRenderImageDescriptorSetLayout.get();
	compute_layout_info.setLayoutCount = 1;

	mBackgroundPipelineLayout = mContext->logical_device->createPipelineLayoutUnique( compute_layout_info );

	auto compute_shader = create_shader_module_from_file( *mContext->logical_device, "./src/core/graphics/shaders/compiled/gradient.comp.spv" );
	if( !compute_shader.has_value() )
	{
		LOG_ERROR( "Failed to create shader module." );
		return false;
	}

	vk::PipelineShaderStageCreateInfo compute_stage_info{};
	compute_stage_info.stage = vk::ShaderStageFlagBits::eCompute;
	compute_stage_info.module = compute_shader.value().get();
	compute_stage_info.pName = "main";

	vk::ComputePipelineCreateInfo compute_pipeline_info{};
	compute_pipeline_info.stage = compute_stage_info;
	compute_pipeline_info.layout = mBackgroundPipelineLayout.get();

	auto result = mContext->logical_device->createComputePipelineUnique( nullptr, compute_pipeline_info );
	if( result.result != vk::Result::eSuccess )
	{
		LOG_ERROR( "Failed to create compute pipeline." );
		return false;
	}

	mBackgroundPipeline = std::move( result.value );
	return true;
}

void
Renderer::InitImGUI( SDL_Window& window )
{
	mImGUILifetime = std::make_unique<ImGUILifetime>( *mContext );
	mImGUILifetime->Init( window, MAX_FRAMES_IN_FLIGHT, static_cast<uint32_t>( mSwapChain->GetImages().size() ), mSwapChain->GetImageFormat() );

	ImmediateSubmit( []( vk::CommandBuffer& )
	{
		if( !ImGui_ImplVulkan_CreateFontsTexture() )
		{
			LOG_ERROR( "Failed to create IMGUI fonts texture." );
		}
	} );
}

void
Renderer::ImmediateSubmit( std::function<void( vk::CommandBuffer& )> work )
{
	auto& cmd = mImmidiateCommandContext->GetPrimaryBuffer();
	mImmidiateCommandContext->Reset();
	mImmidiateCommandContext->Begin();

	work( cmd );

	mImmidiateCommandContext->End();

	auto submit_info = create_submit_info(
		mImmidiateCommandContext->GetSubmitInfo(), std::nullopt, std::nullopt
	);

	mContext->graphics_queue.submit2( submit_info, mImmidiateCommandContext->GetFence() );
	mImmidiateCommandContext->WaitForCompletion();
}

void
Renderer::PrepareImGUI()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	// @TODO: Actual setup of the GUI should be passed in as a callback
	ImGui::ShowDemoWindow();
	ImGui::Render();
}