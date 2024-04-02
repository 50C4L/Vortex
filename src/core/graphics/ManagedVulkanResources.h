#ifndef _MANAGED_VULKAN_RESOURCES_H
#define _MANAGED_VULKAN_RESOURCES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <optional>
#include <memory>
#include <functional>

namespace graphics
{
	class ManagedImage
	{
	public:
		ManagedImage( vk::Device& device, VmaAllocator& allocator, vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage );
		virtual ~ManagedImage();

		vk::Image& GetImage();
		vk::ImageView& GetImageView();
		vk::Extent2D GetExtent2D() const;
		vk::Format GetFormat() const;

	private:
		VmaAllocator& mAllocator;
		VmaAllocation mAllocation;
		VmaAllocationInfo mAllocationInfo;
		vk::Extent3D mExtent;
		vk::Format mFormat;
		vk::Image mImage;
		vk::UniqueImageView mImageView;
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