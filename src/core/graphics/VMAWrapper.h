#ifndef _VMA_WRAPPER_H
#define _VMA_WRAPPER_H

#include <vk_mem_alloc.h>

#include <memory>
#include <functional>

namespace graphics
{
	class VulkanContext;

	struct VMAWrapper
	{
		VMAWrapper( VulkanContext& context );

		std::unique_ptr<VmaAllocator, std::function<void(VmaAllocator*)>> allocator;
	};
}

#endif // _VMA_WRAPPER_H