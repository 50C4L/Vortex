#include "ManagedVulkanResources.h"

#include <utility/Logger.h>

using namespace graphics;
using namespace utility;

namespace
{
	vk::ImageCreateInfo create_image_info( vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, uint32_t mip_levels )
	{
		vk::ImageCreateInfo image_info{};
		image_info.imageType = vk::ImageType::e2D;
		image_info.extent = extent;
		image_info.mipLevels = mip_levels;
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

/*static*/
ManagedImage::Ptr
ManagedImage::Create( 
	vk::Device& device, 
	VmaAllocator& allocator, 
	vk::Extent3D extent, 
	vk::Format format, 
	vk::ImageUsageFlags usage, 
	vk::ImageAspectFlags aspect_flags, 
	uint32_t mip_levels
)
{
	auto image_create_info = create_image_info( extent, format, usage, mip_levels );

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VkMemoryPropertyFlags( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	Ptr ret = Ptr( new ManagedImage(), [&allocator]( ManagedImage* image )
	{
		vmaDestroyImage( allocator, image->image, image->allocation );
		delete image;
	} );

	if( vmaCreateImage(
		allocator,
		reinterpret_cast<VkImageCreateInfo*>( &image_create_info ),
		&alloc_info,
		reinterpret_cast<VkImage*>( &ret->image ),
		&ret->allocation,
		&ret->allocation_info ) != VK_SUCCESS )
	{
		LOG_ERROR( "Failed to create image" );
		throw std::runtime_error( "Failed to create image" );
	}

	// create the image view
	vk::ImageViewCreateInfo image_view_info{};
	image_view_info.image = ret->image;
	image_view_info.viewType = vk::ImageViewType::e2D;
	image_view_info.format = format;
	image_view_info.subresourceRange = vk::ImageSubresourceRange{ aspect_flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };

	ret->image_view = device.createImageViewUnique( image_view_info );
	ret->extent = extent;
	ret->format = format;

	return ret;
}

/*static*/
ManagedBuffer::Ptr
ManagedBuffer::Create( VmaAllocator& allocator, size_t buffer_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage )
{
	vk::BufferCreateInfo buffer_info{};
	buffer_info.size = buffer_size;
	buffer_info.usage = usage;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = memory_usage;
	alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	Ptr buffer = Ptr( new ManagedBuffer( allocator ), [&allocator]( ManagedBuffer* buffer )
	{
		vmaDestroyBuffer( allocator, buffer->buffer, buffer->allocation );
		delete buffer;
	} );

	if( vmaCreateBuffer(
		allocator,
		reinterpret_cast<VkBufferCreateInfo*>( &buffer_info ),
		&alloc_info,
		reinterpret_cast<VkBuffer*>( &buffer->buffer ),
		&buffer->allocation,
		&buffer->allocation_info ) != VK_SUCCESS )
	{
		LOG_ERROR( "Failed to create buffer" );
		throw std::runtime_error( "Failed to create buffer" );
	}

	return buffer;
}

ManagedBuffer::ManagedBuffer( VmaAllocator& allocator )
	: allocator( allocator )
{
}

void*
ManagedBuffer::Map()
{
	return allocation_info.pMappedData;
}

void
ManagedBuffer::Update( void* data, size_t size, uint64_t offset )
{
	void* mapped_data = Map();
	mapped_data = static_cast<uint8_t*>( mapped_data ) + offset;
	memcpy( mapped_data, data, size );
	vmaFlushAllocation( allocator, allocation, offset, size );
}