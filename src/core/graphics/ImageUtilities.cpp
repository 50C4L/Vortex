#include "ImageUtilities.h"

void eage::graphics::transition_image( vk::CommandBuffer& cmd_buffer, vk::Image image, vk::ImageLayout current_layout, vk::ImageLayout new_layout )
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
		( new_layout == vk::ImageLayout::eDepthAttachmentOptimal ) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
	image_barrier.subresourceRange = vk::ImageSubresourceRange{ aspect_mask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
	image_barrier.image = image;

	vk::DependencyInfo dep_info{};
	dep_info.sType = vk::StructureType::eDependencyInfo;
	dep_info.imageMemoryBarrierCount = 1;
	dep_info.pImageMemoryBarriers = &image_barrier;

	cmd_buffer.pipelineBarrier2( dep_info );
}

void eage::graphics::copy_image_to_image( vk::CommandBuffer& cmd_buffer, vk::Image src_image, vk::Image dst_image, vk::Extent2D src_size, vk::Extent2D dst_size )
{
	vk::ImageBlit2 blit_region{};

	blit_region.srcOffsets[1] = vk::Offset3D{ static_cast<int32_t>( src_size.width ), static_cast<int32_t>( src_size.height ), 1 };
	blit_region.dstOffsets[1] = vk::Offset3D{ static_cast<int32_t>( dst_size.width ), static_cast<int32_t>( dst_size.height ), 1 };

	blit_region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	blit_region.srcSubresource.layerCount = 1;
	blit_region.srcSubresource.baseArrayLayer = 0;
	blit_region.srcSubresource.mipLevel = 0;

	blit_region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	blit_region.dstSubresource.layerCount = 1;
	blit_region.dstSubresource.baseArrayLayer = 0;
	blit_region.dstSubresource.mipLevel = 0;

	vk::BlitImageInfo2 blit_info{};
	blit_info.srcImage = src_image;
	blit_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
	blit_info.dstImage = dst_image;
	blit_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
	blit_info.regionCount = 1;
	blit_info.filter = vk::Filter::eLinear;
	blit_info.pRegions = &blit_region;

	cmd_buffer.blitImage2( blit_info );
}