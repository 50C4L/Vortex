#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>

#include <utility/Logger.h>
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

	LOG( "Initializing vulkan swap chain ..." );
	mSwapChain = std::make_unique<VulkanSwapChain>( *mContext, 800, 600 );
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
		mFrames.push_back( Frame{ {}, std::make_unique<VulkanCommandContext>( *mContext ), i } );
	}

	return true;
}

void
Renderer::Render()
{
	auto& frame = GetCurrentFrame();
	auto& cmd = frame.command_context->GetPrimaryBuffer();
	auto& image = mSwapChain->GetImages()[ frame.index ];

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

	Present();

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
		mContext->GetSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput, VulkanContext::SemaphoreType::WAIT ),
		mContext->GetSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput, VulkanContext::SemaphoreType::SIGNAL )
	);

	mContext->graphics_queue.submit2( submit_info, frame.command_context->GetFence() );
}

void
Renderer::Present()
{
	auto& frame = GetCurrentFrame();
	auto& image = mSwapChain->GetImages()[ frame.index ];

	vk::PresentInfoKHR present_info{};
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &mContext->image_available_semaphore.get();
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &mSwapChain->GetSwapChain();
	present_info.pImageIndices      = &frame.index;

	mContext->present_queue.presentKHR( present_info );
}