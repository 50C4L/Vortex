#ifndef _EAGE_VULKAN_SWAP_CHAIN_H
#define _EAGE_VULKAN_SWAP_CHAIN_H

#include <vulkan/vulkan_raii.hpp>
#include <vector>

struct GLFWwindow;

namespace eage::graphics
{
	// class VMAManagedImage;
	class VulkanContext;

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain( VulkanContext& context, uint32_t width, uint32_t height );
		virtual ~VulkanSwapChain();

		uint32_t GetNextImage( vk::Semaphore senmaphore );
		void PresentToQueue( vk::Queue present_qeuue, uint32_t image_index, vk::Semaphore semaphore );

		vk::Format GetImageFormat() { return mFormat; }
		vk::Format GetDepthFormat() { return mDepthFormat; }
		vk::Extent2D GetExtent() { return mExtent; }
		std::vector<vk::Image>& GetImages() { return mImages; }
		const vk::ImageView& GetDepthImageView() const { return *mDepthImageView; }
		const vk::SwapchainKHR& GetSwapChain() { return *mSwapChain; }
		std::vector<vk::raii::ImageView>& GetImageViews() { return mImageViews; }

	private:
		// void CreateDepthImages( VmaAllocator allocator );
		// vk::Format FindSupportedFormat( const std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags feature_flags );
		VulkanContext& mContext;

		vk::raii::SwapchainKHR           mSwapChain{ nullptr };
		std::vector<vk::Image>           mImages;
		vk::Format                       mFormat;
		vk::Extent2D                     mExtent;
		std::vector<vk::raii::ImageView> mImageViews;

		// Depth and Stencil
		vk::Format mDepthFormat;
		// std::unique_ptr<VMAManagedImage>     mDepthImage;
		vk::raii::ImageView                  mDepthImageView{ nullptr };
	};
}

#endif // _EAGE_VULKAN_SWAP_CHAIN_H

