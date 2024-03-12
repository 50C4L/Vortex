#include "VMAWrapper.h"

#include <graphics/VulkanContext.h>

#define VMA_IMPLEMENTATION

using namespace graphics;

VMAWrapper::VMAWrapper( VulkanContext& context )
{
	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.physicalDevice = context.physical_device;
	allocator_info.device         = context.logical_device.get();
	allocator_info.instance       = context.instance.get();

	VmaAllocator* vma_allocator = new VmaAllocator();
	vmaCreateAllocator( &allocator_info, vma_allocator );

	allocator = std::unique_ptr<VmaAllocator, std::function<void(VmaAllocator*)>>(
		std::move( vma_allocator ),
		[]( VmaAllocator* allocator ) { vmaDestroyAllocator( *allocator ); }
	);
}