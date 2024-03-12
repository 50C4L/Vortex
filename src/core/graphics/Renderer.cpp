#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_system.h>
#include <iostream>

#include <utility/Logger.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanContext.h>
#include <graphics/VulkanSwapChain.h>
#include <graphics/VulkanCommandContext.h>
#include <graphics/VMAWrapper.h>

using namespace graphics;
using namespace utility;

namespace
{
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2; // Double buffering

	void transition_image( vk::CommandBuffer& cmd_buffer, vk::Image image, vk::ImageLayout current_layout, vk::ImageLayout new_layout )
	{
		vk::ImageMemoryBarrier2 image_barrier{};
		image_barrier.sType = vk::StructureType::eImageMemoryBarrier2;

		image_barrier.oldLayout = current_layout;
		image_barrier.newLayout = new_layout;

		image_barrier.srcStageMask  = vk::PipelineStageFlagBits2::eAllCommands; //< This means it blocks all GPU commands, can be optimized
		image_barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
		image_barrier.dstStageMask  = vk::PipelineStageFlagBits2::eAllCommands;
		image_barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;

		vk::ImageAspectFlags aspect_mask =
			( new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal ) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
		image_barrier.subresourceRange = vk::ImageSubresourceRange{ aspect_mask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		image_barrier.image = image;

		vk::DependencyInfo dep_info{};
		dep_info.sType = vk::StructureType::eDependencyInfo;
		dep_info.imageMemoryBarrierCount = 1;
		dep_info.pImageMemoryBarriers = &image_barrier;

		cmd_buffer.pipelineBarrier2( dep_info );
	}

	vk::SubmitInfo2 create_submit_info( vk::CommandBufferSubmitInfo cmd_submit_info, vk::SemaphoreSubmitInfo semaphore_wait_info, vk::SemaphoreSubmitInfo semaphore_signal_info )
	{
		vk::SubmitInfo2 submit_info{};
		submit_info.waitSemaphoreInfoCount = 1;
		submit_info.pWaitSemaphoreInfos = &semaphore_wait_info;
		submit_info.signalSemaphoreInfoCount = 1;
		submit_info.pSignalSemaphoreInfos = &semaphore_signal_info;
		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &cmd_submit_info;

		return submit_info;
	}

	vk::ImageCreateInfo create_image_info( vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage )
	{
		vk::ImageCreateInfo image_info{};
		image_info.imageType = vk::ImageType::e2D;
		image_info.extent = extent;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = vk::ImageTiling::eOptimal;
		image_info.usage = usage;
		image_info.samples = vk::SampleCountFlagBits::e1;

		return image_info;
	}

	vk::ImageViewCreateInfo create_image_view_info( vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags )
	{
		vk::ImageViewCreateInfo image_view_info{};
		image_view_info.image = image;
		image_view_info.viewType = vk::ImageViewType::e2D;
		image_view_info.format = format;
		image_view_info.subresourceRange = vk::ImageSubresourceRange{ aspect_flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };

		return image_view_info;
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

	LOG( "Creating render image ..." );
	mRenderImage = std::make_unique<AllocatedImage>();
	vk::Extent3D extent{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 };
	mRenderImage->format = vk::Format::eR16G16B16A16Sfloat;
	mRenderImage->extent = extent;

	vk::ImageUsageFlags image_usage_flags;
	image_usage_flags |= vk::ImageUsageFlagBits::eTransferSrc;
	image_usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
	image_usage_flags |= vk::ImageUsageFlagBits::eStorage;
	image_usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;

	auto render_image_create_info = create_image_info( extent, mRenderImage->format, image_usage_flags );
	VmaAllocationCreateInfo image_alloc_info{};
	image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	image_alloc_info.requiredFlags = VkMemoryPropertyFlags( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	// allocate and create the image
	vmaCreateImage( 
		*mVMA->allocator.get(), 
		reinterpret_cast<VkImageCreateInfo*>( &render_image_create_info ), 
		&image_alloc_info, 
		reinterpret_cast<VkImage*>( &mRenderImage->image.get() ), &mRenderImage->allocation, nullptr );

	// create the image view
	auto image_view_info = create_image_view_info( mRenderImage->image.get(), mRenderImage->format, vk::ImageAspectFlagBits::eColor );
	mRenderImage->image_view = mContext->logical_device->createImageViewUnique( image_view_info );

	return true;
}

void
Renderer::Render()
{
	auto& frame = GetCurrentFrame();
	auto& cmd = frame.command_context->GetPrimaryBuffer();
	uint32_t next_image_index = mSwapChain->GetNextImage( frame.command_context->GetSwapchainSemaphore() );
	auto& image = mSwapChain->GetImages()[ next_image_index ];

	frame.command_context->WaitForCompletion();
	frame.command_context->Reset();
	frame.command_context->Begin();

	transition_image( cmd, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral );

	vk::ClearColorValue clear_color;
	float flash = abs( sin( mFrameNumber / 100.0f ) );
	clear_color.setFloat32( { 0.0f, 0.0f, flash, 1.0f } );

	vk::ImageSubresourceRange clear_range{ vk::ImageAspectFlagBits::eColor, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };

	cmd.clearColorImage( image, vk::ImageLayout::eGeneral, clear_color, clear_range );

	transition_image( cmd, image, vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR );

	frame.command_context->End();

	/* TODO
	vulkan_main_buffer->begin();
	vulkan_main_buffer->clear( 0.0f, 0.0f, 0.0f, 1.0f );

	for( const auto& renderable : GetCurrentFrame().mRenderables )
	{
		renderable.Render( *vulkan_main_buffer );
	}

	vulkan_main_buffer->end();
	*/

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

	mContext->present_queue.presentKHR( present_info );
}