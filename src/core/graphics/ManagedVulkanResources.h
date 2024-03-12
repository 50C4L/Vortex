#ifndef _MANAGED_VULKAN_RESOURCES_H
#define _MANAGED_VULKAN_RESOURCES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace graphics
{
	struct AllocatedImage
	{
		vk::UniqueImage image;
		vk::UniqueImageView image_view;
		VmaAllocation allocation;
		vk::Extent3D extent;
		vk::Format format;
	};
}

#endif // _MANAGED_VULKAN_RESOURCES_H