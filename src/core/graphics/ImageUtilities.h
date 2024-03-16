#ifndef _IMAGE_UTILITIES_H
#define _IMAGE_UTILITIES_H

#include <vulkan/vulkan.hpp>

namespace graphics
{
	void transition_image( vk::CommandBuffer& cmd_buffer, vk::Image image, vk::ImageLayout current_layout, vk::ImageLayout new_layout );

	void copy_image_to_image( vk::CommandBuffer& cmd_buffer, vk::Image src_image, vk::Image dst_image, vk::Extent2D src_size, vk::Extent2D dst_size );
}

#endif // _IMAGE_UTILITIES_H