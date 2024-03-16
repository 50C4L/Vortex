#include "ManagedVulkanResources.h"

using namespace graphics;

namespace
{
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

ManagedImage::ManagedImage( vk::Device& device, VmaAllocator& allocator, vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage )
	: mAllocator( allocator )
	, mExtent( extent )
	, mFormat( format )
{
	auto render_image_create_info = create_image_info( extent, format, usage );

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VkMemoryPropertyFlags( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	// allocate and create the image
	vmaCreateImage(
		allocator,
		reinterpret_cast<VkImageCreateInfo*>( &render_image_create_info ),
		&alloc_info,
		reinterpret_cast<VkImage*>( &mImage ),
		&mAllocation,
		nullptr );

	// create the image view
	auto image_view_info = create_image_view_info( mImage, format, vk::ImageAspectFlagBits::eColor );
	mImageView = device.createImageViewUnique( image_view_info );
}

ManagedImage::~ManagedImage()
{
	vmaDestroyImage( mAllocator, mImage, mAllocation );
}

vk::Image& ManagedImage::GetImage()
{
	return mImage;
}

vk::ImageView& ManagedImage::GetImageView()
{
	return mImageView.get();
}

vk::Extent2D ManagedImage::GetExtent2D() const
{
	return { mExtent.width, mExtent.height };
}