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

		vk::UniqueInstance						instance;
		std::unique_ptr<VulkanDebugMessenger>	debug_messenger;
		vk::UniqueSurfaceKHR					surface;
		vk::PhysicalDevice						physical_device;
		QueueFamilyIndices						queue_indices;
	};
} // namespace graphics


#endif // _EAGE_VULKAN_CONTEXT_H