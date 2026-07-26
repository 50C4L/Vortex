#include "PresentPass.h"

#include <array>

#include <graphics/ImageUtilities.h>
#include <graphics/ManagedVulkanResources.h>

using namespace eage::graphics;

PresentPass::PresentPass()
{
	mDesc.color_target = nullptr;
	mDesc.depth_target = nullptr;
	mDesc.swapchain_access = SwapchainAccess::TransferDst;
}

void
PresentPass::SetSource( ManagedImage* image )
{
	mSource = image;
	mDesc.input_images.clear();

	if( mSource != nullptr )
	{
		mDesc.input_images.push_back( PassInput{ mSource, ImageAccess::TransferSrc } );
	}
}

const RenderPassDesc&
PresentPass::GetDesc() const
{
	return mDesc;
}

void
PresentPass::Execute( CommandBuffer& buffer, const FrameContext& ctx )
{
	vk::CommandBuffer cmd( static_cast<VkCommandBuffer>( buffer.GetNativeHandle() ) );
	vk::Image swapchain_image( static_cast<VkImage>( ctx.swapchain_image_handle ) );
	vk::Extent2D swapchain_extent{ ctx.swapchain_width, ctx.swapchain_height };

	if( mSource != nullptr )
	{
		vk::Extent2D src_extent{ mSource->extent.width, mSource->extent.height };
		copy_image_to_image(
			cmd,
			mSource->image,
			swapchain_image,
			src_extent,
			swapchain_extent,
			vk::Filter::eNearest );
	}
	else
	{
		vk::ClearColorValue clear_color{ std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f } };
		vk::ImageSubresourceRange range{
			vk::ImageAspectFlagBits::eColor,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS };
		cmd.clearColorImage( swapchain_image, vk::ImageLayout::eTransferDstOptimal, clear_color, range );
	}
}
