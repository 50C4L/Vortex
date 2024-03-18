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

#include <imgui/imgui.h>
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
}


Renderer::Renderer()
	: mFrameNumber( 0 )
	, mIMGUIInitialized( false )
{
}

Renderer::~Renderer()
{
	if( mIMGUIInitialized )
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
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
	mIMGUIInitialized = InitIMGUI( window );

	return true;
}

void
Renderer::Render()
{
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

	// Transition the swapchain image back to the present layout
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR );

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

bool
Renderer::InitIMGUI( SDL_Window& window )
{
	LOG( "Initializing IMGUI ..." );

	const uint32_t max_sets = 1000;
	vk::DescriptorPoolSize pool_sizes[] =
	{
		{ vk::DescriptorType::eSampler, max_sets },
		{ vk::DescriptorType::eCombinedImageSampler, max_sets },
		{ vk::DescriptorType::eSampledImage, max_sets },
		{ vk::DescriptorType::eStorageImage, max_sets },
		{ vk::DescriptorType::eUniformTexelBuffer, max_sets },
		{ vk::DescriptorType::eStorageTexelBuffer, max_sets },
		{ vk::DescriptorType::eUniformBuffer, max_sets },
		{ vk::DescriptorType::eStorageBuffer, max_sets },
		{ vk::DescriptorType::eUniformBufferDynamic, max_sets },
		{ vk::DescriptorType::eStorageBufferDynamic, max_sets },
		{ vk::DescriptorType::eInputAttachment, max_sets }
	};

	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.poolSizeCount = static_cast<uint32_t>( std::size( pool_sizes ) );
	pool_info.pPoolSizes = pool_sizes;
	pool_info.maxSets = max_sets;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

	vk::DescriptorPool descriptor_pool = mContext->logical_device->createDescriptorPool( pool_info );

	ImGui::CreateContext();

	if( !ImGui_ImplSDL2_InitForVulkan( &window ) )
	{
		LOG_ERROR( "Failed to initialize IMGUI for SDL2" );
		return false;
	}

	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = mContext->instance.get();
	init_info.PhysicalDevice = mContext->physical_device;
	init_info.Device = mContext->logical_device.get();
	init_info.Queue = mContext->graphics_queue;
	init_info.DescriptorPool = std::move( descriptor_pool );
	init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
	init_info.ImageCount = static_cast<uint32_t>( mSwapChain->GetImages().size() );
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineRenderingCreateInfo = std::move( pipeline_rendering_create_info );
	
	
	if( !ImGui_ImplVulkan_Init( &init_info ) )
	{
		LOG_ERROR( "Failed to initialize IMGUI for Vulkan." );
		return false;
	}

	ImmediateSubmit( []( vk::CommandBuffer& )
	{
		ImGui_ImplVulkan_CreateFontsTexture();
	} );

	return true;
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