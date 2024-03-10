#include "VulkanSwapChain.h"

#include "VulkanContext.h"
// #include "VulkanBuffers.h"
// #include "VulkanTools.h"

using namespace graphics;

namespace
{
	///
	/// Swap chain wrapped data
	/// 
	struct SwapChainSupportDetails 
	{
		vk::SurfaceCapabilitiesKHR        capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR>   present_modes;

		bool IsComplete()
		{
			return !formats.empty() && !present_modes.empty();
		}
	};

	///
	/// Query for swap chain support information from the given pyhsical device
	/// 
	/// @param devicef
	///  Const reference to vk::PhysicalDevice
	/// 
	/// @return SwapChainSupportDetails
	///
	SwapChainSupportDetails query_swap_chain_support( const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface )
	{
		SwapChainSupportDetails details;

		details.capabilities  = device.getSurfaceCapabilitiesKHR( surface );
		details.formats       = device.getSurfaceFormatsKHR( surface );
		details.present_modes = device.getSurfacePresentModesKHR( surface );

		return details;
	}

	///
	/// Try to find the best surface format based on the requirement
	/// @TODO: Consider requirements that's passed from the outside
	/// 
	/// @param available_formats
	///  The available formats for the surface
	/// 
	/// @return
	///  The best or the first from the list
	/// 
	vk::SurfaceFormatKHR get_best_swap_surface_format( const std::vector<vk::SurfaceFormatKHR> &available_formats )
	{
		for( const auto& format : available_formats )
		{
			if( format.format == vk::Format::eA8B8G8R8SrgbPack32 && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear )
			{
				return format;
			}
		}
		return available_formats.front();
	}

	///
	/// Try to find the desired present mode from the available present modes
	/// 
	/// @target_mode
	///  The desired present mode, can be one of the followings
	///  - VK_PRESENT_MODE_IMMEDIATE_KHR (VSync off)
	///  - VK_PRESENT_MODE_FIFO_KHR (VSync on) (Tripple buffering)
	///  - VK_PRESENT_MODE_FIFO_RELAXED_KHR (Adaptive sync)
	///  - VK_PRESENT_MODE_MAILBOX_KHR (Double buffering)
	/// 
	/// @param available_modes
	///  The available present modes for the surface
	/// 
	/// @return 
	///  The desired mode or VK_PRESENT_MODE_FIFO_KHR
	/// 
	vk::PresentModeKHR choose_swap_present_mode( vk::PresentModeKHR target_mode, const std::vector<vk::PresentModeKHR> &available_modes )
	{
		for( const auto& mode : available_modes )
		{
			if( mode == target_mode )
			{
				return mode;
			}
		}
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D choose_swap_extent( const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height ) 
	{
		if( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
		{
			return capabilities.currentExtent;
		}
		else
		{
			width  = std::clamp( width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
			height = std::clamp( height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

			return { width, height };
		}
	}
}

VulkanSwapChain::VulkanSwapChain( VulkanContext& context, uint32_t width, uint32_t height )
: mContext( context )
{
	auto sc_support = query_swap_chain_support( mContext.physical_device, mContext.surface.get() );
	if( !sc_support.IsComplete() )
	{
		throw std::runtime_error( "Physical device doesn't have swap chain support!" );
	}

	vk::SurfaceFormatKHR surface_format = get_best_swap_surface_format( sc_support.formats );
	mFormat = surface_format.format;
	vk::PresentModeKHR present_mode = choose_swap_present_mode( vk::PresentModeKHR::eFifo, sc_support.present_modes );
	mExtent = choose_swap_extent( sc_support.capabilities, width, height );

	std::vector<uint32_t> queue_fam_indices;
	queue_fam_indices.push_back( mContext.queue_indices.graphics_family.value() );
	if( mContext.queue_indices.graphics_family != mContext.queue_indices.present_family )
	{
		queue_fam_indices.push_back( mContext.queue_indices.present_family.value() );
	}

	vk::SwapchainCreateInfoKHR create_info{};
	create_info.setSurface( mContext.surface.get() );
	create_info.setMinImageCount( 2 );
	create_info.setImageFormat( surface_format.format );
	create_info.setImageColorSpace( surface_format.colorSpace );
	create_info.setImageExtent( mExtent );
	constexpr uint32_t image_array_layers = 1;
	create_info.setImageArrayLayers( image_array_layers );
	create_info.setImageUsage( vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment );
	create_info.presentMode = present_mode;
	create_info.imageSharingMode = queue_fam_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
	create_info.queueFamilyIndexCount = static_cast<uint32_t>( queue_fam_indices.size() );
	create_info.pQueueFamilyIndices = reinterpret_cast<const uint32_t*>( queue_fam_indices.data() );
	create_info.preTransform = sc_support.capabilities.currentTransform;
	create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	try
	{
		mSwapChain = mContext.logical_device->createSwapchainKHRUnique( create_info );
	}
	catch ( vk::SystemError /*err*/ )
	{
		throw std::runtime_error( "Failed to create swap chain." );
	}

	try
	{
		mImages = mContext.logical_device->getSwapchainImagesKHR( mSwapChain.get() );
	}
	catch ( vk::SystemError /*err*/ )
	{
		throw std::runtime_error( "Failed to get sawp chain images." );
	}

	// Create image views
	mImageViews.resize( mImages.size() );
	for( auto i = 0; i < mImages.size(); i++ )
	{
		vk::ImageViewCreateInfo view_info{};
		view_info.image = mImages[ i ];
		view_info.viewType = vk::ImageViewType::e2D;
		view_info.format = mFormat;
		view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;

		try
		{
			mImageViews[ i ] = context.logical_device->createImageViewUnique( view_info );
		}
		catch ( vk::SystemError /*err*/ )
		{
			throw std::runtime_error( "Failed to get sawp chain images." );
		}
	}

	// CreateDepthImages( std::move( allocator ) );
}

VulkanSwapChain::~VulkanSwapChain()
{
}

uint32_t 
VulkanSwapChain::GetNextImage( vk::Semaphore& senmaphore )
{
	auto res = mContext.logical_device->acquireNextImageKHR( mSwapChain.get(), UINT64_MAX, senmaphore, VK_NULL_HANDLE );
	if( res.result == vk::Result::eSuccess )
	{
		return res.value;
	}
	else
	{
		throw std::runtime_error( "Failed to acquire the next image from the swap chain." );
	}
}

void 
VulkanSwapChain::PresentToQueue( vk::Queue& present_qeuue, uint32_t image_index, vk::Semaphore& semaphore )
{
	vk::PresentInfoKHR present_info{};
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &mSwapChain.get();
	present_info.pImageIndices = &image_index;
	present_info.waitSemaphoreCount = 1;

	if( semaphore )
	{
		present_info.pWaitSemaphores = &semaphore;
		present_info.waitSemaphoreCount = 1;
	}
	if( present_qeuue.presentKHR( present_info ) != vk::Result::eSuccess )
	{
		throw std::runtime_error( "Failed to present swap chain image to the given queue!" );
	}
}

// void 
// VulkanSwapChain::CreateDepthImages( VmaAllocator allocator )
// {
// 	auto result = GetSupportDepthFormat( mPhysicalDevice );
// 	if( !result.success )
// 	{
// 		throw std::runtime_error( "Failed to find a valid depth format!" );
// 	}
// 	mDepthFormat = std::get<vk::Format>( result.result );

// 	mDepthImage = std::make_unique<VMAManagedImage>( allocator, mExtent.width, mExtent.height, mDepthFormat,vk::ImageUsageFlagBits::eDepthStencilAttachment );

// 	// Create image view
// 	vk::ImageViewCreateInfo view_info{};
// 	view_info.image    = mDepthImage->GetImage();
// 	view_info.viewType = vk::ImageViewType::e2D;
// 	view_info.format   = mDepthFormat;
// 	view_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
// 	view_info.subresourceRange.baseMipLevel   = 0;
// 	view_info.subresourceRange.levelCount     = 1;
// 	view_info.subresourceRange.baseArrayLayer = 0;
// 	view_info.subresourceRange.layerCount     = 1;
// 	if( mDepthFormat >= vk::Format::eD16UnormS8Uint ) 
// 	{
// 		view_info.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
// 	}

// 	mDepthImageView = mLogicalDevice.createImageViewUnique( view_info );
// }

// vk::Format 
// VulkanSwapChain::FindSupportedFormat( const std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags feature_flags )
// {
// 	for( vk::Format format : formats ) 
// 	{
// 		vk::FormatProperties props = mPhysicalDevice.getFormatProperties( format );

// 		if( tiling == vk::ImageTiling::eLinear && ( props.linearTilingFeatures & feature_flags ) == feature_flags ) 
// 		{
// 			return format;
// 		} 
// 		else if( tiling == vk::ImageTiling::eOptimal && ( props.optimalTilingFeatures & feature_flags ) == feature_flags )
// 		{
// 			return format;
// 		}
// 	}

// 	throw std::runtime_error( "failed to find supported format!" );
// }
