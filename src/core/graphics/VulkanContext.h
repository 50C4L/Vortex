#ifndef _EAGE_VULKAN_CONTEXT_H
#define _EAGE_VULKAN_CONTEXT_H

#include <vulkan/vulkan_raii.hpp>
#include <optional>

struct SDL_Window;

namespace eage::graphics
{
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

		// Declared first - must outlive all other handles (reverse destruction order)
		vk::raii::Context						raii_context;
		vk::raii::Instance						instance{ nullptr };
		vk::raii::DebugUtilsMessengerEXT		debug_messenger{ nullptr };
		vk::raii::SurfaceKHR					surface{ nullptr };
		vk::raii::PhysicalDevice				physical_device{ nullptr };
		vk::raii::Device						logical_device{ nullptr };
		QueueFamilyIndices						queue_indices;
		vk::raii::Queue							graphics_queue{ nullptr };
		vk::raii::Queue							present_queue{ nullptr };
	};
} // namespace eage::graphics


#endif // _EAGE_VULKAN_CONTEXT_H