#ifndef _MANAGED_VULKAN_RESOURCES_H
#define _MANAGED_VULKAN_RESOURCES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace graphics
{
	class ManagedImage
	{
	public:
		ManagedImage( vk::Device& device, VmaAllocator& allocator, vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage );
		virtual ~ManagedImage();

		vk::Image& GetImage();
		vk::ImageView& GetImageView();

	private:
		VmaAllocator& mAllocator;
		VmaAllocation mAllocation;
		vk::Extent3D mExtent;
		vk::Format mFormat;
		vk::Image mImage;
		vk::UniqueImageView mImageView;
	};
}

#endif // _MANAGED_VULKAN_RESOURCES_H