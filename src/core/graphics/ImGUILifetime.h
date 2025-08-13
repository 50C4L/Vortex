#ifndef _IMGUI_LIFETIME_H
#define _IMGUI_LIFETIME_H

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace eage::graphics
{
	class VulkanContext;

	class ImGUILifetime
	{
	public:
		///
		/// Constructor
		///
		ImGUILifetime( VulkanContext& context );

		///
		/// Destructor
		///
		virtual ~ImGUILifetime();

		///
		/// Initialize the ImGui wrapper
		///
		/// @return
		///  true if successful, false otherwise
		///
		bool Init( SDL_Window& window, uint32_t min_image_count, uint32_t max_image_count, vk::Format format );

	private:
		VulkanContext& mContext;
		vk::UniqueDescriptorPool mDescriptorPool;
	};
}

#endif // _IMGUI_LIFETIME_H