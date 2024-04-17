#ifndef _MANAGED_VULKAN_RESOURCES_H
#define _MANAGED_VULKAN_RESOURCES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <optional>
#include <memory>
#include <functional>

namespace graphics
{
	struct ManagedImage
	{
		using Ptr = std::unique_ptr<ManagedImage, std::function<void(ManagedImage*)>>;
		static Ptr Create( vk::Device& device, VmaAllocator& allocator, vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels = 1 );

		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;
		vk::Extent3D extent;
		vk::Format format;
		vk::Image image;
		vk::UniqueImageView image_view;
	};

	struct ManagedBuffer
	{
		using Ptr = std::unique_ptr<ManagedBuffer, std::function<void(ManagedBuffer*)>>;
		static Ptr Create( VmaAllocator& allocator, size_t buffer_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage );

		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;
		vk::Buffer buffer;
	};
}

#endif // _MANAGED_VULKAN_RESOURCES_H