#ifndef _EAGE_VULKAN_CONTEXT_H
#define _EAGE_VULKAN_CONTEXT_H

#include <vulkan/vulkan.hpp>
#include <optional>

struct SDL_Window;

namespace graphics
{
	class VulkanDebugMessenger;

	class VulkanContext
	{
	public:
		VulkanContext( SDL_Window& window );
		virtual ~VulkanContext();

		/// Wrap all the indices of queue families we need
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics_family;
			std::optional<uint32_t> present_family;

			bool IsComplete()
			{
				return graphics_family.has_value() &&
				       present_family.has_value();
			}
		};

		enum SemaphoreType
		{
			WAIT,
			SIGNAL
		};
		///
		/// Get the queue family indices
		///
		/// @param stage_mask
		///  The stage mask to use
		///
		/// @param type
		///  The semaphore type
		///
		vk::SemaphoreSubmitInfo GetSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask, SemaphoreType type ) const;
		
		vk::UniqueInstance						instance;
		std::unique_ptr<VulkanDebugMessenger>	debug_messenger;
		vk::UniqueSurfaceKHR					surface;
		vk::PhysicalDevice						physical_device;
		vk::UniqueDevice						logical_device;
		QueueFamilyIndices						queue_indices;
		vk::Queue								graphics_queue;
		vk::Queue								present_queue;
		vk::UniqueSemaphore						image_available_semaphore;
		vk::UniqueSemaphore						render_finsihed_semaphore;
	};
} // namespace graphics


#endif // _EAGE_VULKAN_CONTEXT_H