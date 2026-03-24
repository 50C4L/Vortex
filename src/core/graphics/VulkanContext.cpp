#include "VulkanContext.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include <set>

#include <utility/Logger.h>

using namespace eage::graphics;
using namespace utility;

namespace
{
#ifdef NDEBUG
	const bool ENABLE_VALIDATION_LAYERS = false;
#else
	const bool ENABLE_VALIDATION_LAYERS = true;
#endif

	const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

	const std::vector<const char*> DEVICE_EXTENSIONS = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
	};

	///
	/// Check if vulkan validation layer is supported for debug configuration
	/// 
	bool check_validation_layer_support()
	{
		auto available_layers = vk::enumerateInstanceLayerProperties();

		// Check if layer names in `VALIDATION_LAYERS` are supported
		for( const char* layer_name : VALIDATION_LAYERS )
		{
			bool found_layer = false;
			for( const auto& layer : available_layers )
			{
				if( strcmp( layer_name, layer.layerName ) == 0) 
				{
					found_layer = true;
					break;
				}
			}

			if( !found_layer ) 
			{
				return false;
			}
			LOG( layer_name + std::string{" is supported."} );
		}

		return true;
	}

	///
	/// debug callback that is invoked by the Vulkan's validation layer
	/// 
	VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data )
	{
		const std::string msg = "Vulkan Debug | " + std::string{ callback_data->pMessage };
		switch( message_severity )
		{
		case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			LOG_ERROR( msg );
			break;
		default:
			LOG( msg );
			break;
		}

		return VK_FALSE;
	}

	///
	/// Creates and returns a `DebugUtilsMessengerCreateInfoEXT` for debug settings
	///
	vk::DebugUtilsMessengerCreateInfoEXT make_debug_info()
	{
		return vk::DebugUtilsMessengerCreateInfoEXT{
			{},
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debug_callback,
			nullptr };
	}

	///
	/// Find the indices of required queue family from the given device
	///
	/// @param device
	///  Reference to the target device
	/// 
	/// "param surface
	///  Vulkan surface for the target window
	/// 
	/// @return QueueFamilyIndices
	/// 
	VulkanContext::QueueFamilyIndices find_queue_families( const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface )
	{
		VulkanContext::QueueFamilyIndices indices;

		auto queue_families = device.getQueueFamilyProperties();
		int i = 0;
		for( const auto& queue_family : queue_families )
		{
			if( queue_family.queueFlags & vk::QueueFlagBits::eGraphics )
			{
				indices.graphics_family = i;
			}
			if( device.getSurfaceSupportKHR( i, surface ) )
			{
				indices.present_family = i;
			}

			if( indices.IsComplete() )
			{
				break;
			}

			i++;
		}
		return indices; 
	}

	///
	/// Check if the given device has all required extensions supported
	/// 
	/// @param devicef
	///  Const reference to vk::PhysicalDevice
	/// 
	/// @return bool
	///  True means all supported.
	/// 
	bool check_device_extension_support( const vk::PhysicalDevice &device )
	{
		std::set<std::string> required_extenssions( DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end() );
		auto available_extensions = device.enumerateDeviceExtensionProperties();
		for( const auto& extension : available_extensions )
		{
			required_extenssions.erase( extension.extensionName );
		}

		return required_extenssions.empty();
	}

	vk::raii::Instance create_vulkan_instance( vk::raii::Context& raii_context, SDL_Window& window )
	{
		if( ENABLE_VALIDATION_LAYERS && !check_validation_layer_support() ) 
		{
			throw std::runtime_error( "Vulkan validation layer is not available!" );
		}

		LOG( "Creating Vulkan instance ..." );
		vk::ApplicationInfo app_info(
			"Vulkan Triangle",
			VK_MAKE_VERSION( 1, 3, 0 ),
			"Invisible Engine",
			VK_MAKE_VERSION( 1, 3, 0 ),
			VK_API_VERSION_1_3
		);

		// Get supported extensions
		uint32_t supported_extension_count = 0;
		if( !SDL_Vulkan_GetInstanceExtensions( &window, &supported_extension_count, NULL ) )
		{
			throw std::runtime_error( "Failed to get Vulkan instance extensions!" );
		}
		std::vector<const char*> extensions;
		extensions.resize( supported_extension_count );
		SDL_Vulkan_GetInstanceExtensions( &window, &supported_extension_count, extensions.data() );
		if( ENABLE_VALIDATION_LAYERS )
		{
			extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		}

		// Create debug layer if required
		auto debug_info = make_debug_info();
		vk::InstanceCreateInfo create_info(
			{},
			&app_info,
			ENABLE_VALIDATION_LAYERS ? static_cast<uint32_t>( VALIDATION_LAYERS.size() ) : 0,
			ENABLE_VALIDATION_LAYERS ? VALIDATION_LAYERS.data() : nullptr,
			static_cast<uint32_t>( extensions.size() ),
			extensions.data(),
			ENABLE_VALIDATION_LAYERS ? &debug_info : nullptr
		);

		vk::raii::Instance instance( raii_context, create_info );
		LOG( "OK" );

		LOG( "Retrieving Vulkan extensions:" );
		auto ins_extensions = vk::enumerateInstanceExtensionProperties();

		for( const auto& extension : ins_extensions )
		{
			LOG( '\t' + extension.extensionName );
		}
		
		return instance; 
	}

	vk::raii::SurfaceKHR
	create_vulkan_surface( vk::raii::Instance& instance, SDL_Window& window )
	{
		VkSurfaceKHR surface;
		if( !SDL_Vulkan_CreateSurface( &window, *instance, &surface ) )
		{
			throw std::runtime_error( "Failed to create Vulkan surface!" );
		}
		return vk::raii::SurfaceKHR( instance, surface );
	}

	vk::raii::PhysicalDevice
	create_physical_device( vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface, VulkanContext::QueueFamilyIndices& queue_indices )
	{
		vk::raii::PhysicalDevices devices( instance );
		if( devices.size() == 0 )
		{
			throw std::runtime_error( "Failed to find an GPU with Vulkan support!" );
		}

		// Find a suitable device
		for( auto& device : devices )
		{
			auto queue_indcies = find_queue_families( *device, *surface );
			if( queue_indcies.IsComplete() )
			{
				queue_indices = std::move( queue_indcies );
				if( !check_device_extension_support( *device ) )
				{
					continue;
				}
				return std::move( device );
			}
		}

		throw std::runtime_error( "No suitable GPU found with required queue families and extensions!" );
	}

	vk::raii::Device create_logical_device( vk::raii::PhysicalDevice& physical_device, VulkanContext::QueueFamilyIndices& queue_indices )
	{
		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { 
			queue_indices.graphics_family.value(),
			queue_indices.present_family.value()
		};

		constexpr uint32_t queue_count = 1;
		constexpr float queue_priority = 1.0f;
		for( uint32_t queue_fam : unique_queue_families )
		{
			queue_create_infos.push_back( vk::DeviceQueueCreateInfo(
				vk::DeviceQueueCreateFlags(),
				queue_indices.graphics_family.value(),
				queue_count,
				&queue_priority
			) );
		}
		
		// Check required features
		vk::PhysicalDeviceVulkan12Features features_12{};
		vk::PhysicalDeviceVulkan13Features features_13{};
		features_13.pNext = &features_12;

		vk::PhysicalDeviceFeatures2 physical_feature{};
		physical_feature.pNext = &features_13;
		vk::PhysicalDevice raw_physical = *physical_device;
		raw_physical.getFeatures2( &physical_feature );

		if( features_12.bufferDeviceAddress == VK_FALSE ||
			features_12.descriptorIndexing == VK_FALSE ||
			features_12.shaderSampledImageArrayNonUniformIndexing == VK_FALSE || 
			features_12.descriptorBindingSampledImageUpdateAfterBind == VK_FALSE ||
			features_12.shaderUniformBufferArrayNonUniformIndexing == VK_FALSE ||
			features_12.descriptorBindingUniformBufferUpdateAfterBind == VK_FALSE ||
			features_12.shaderStorageBufferArrayNonUniformIndexing == VK_FALSE ||
			features_12.descriptorBindingStorageBufferUpdateAfterBind == VK_FALSE )
		{
			LOG_ERROR( "Vulkan 1.2 features are not supported!" );
		}

		if( features_13.dynamicRendering == VK_FALSE ||
			features_13.synchronization2 == VK_FALSE )
		{
			LOG_ERROR( "Vulkan 1.3 features are not supported!" );
		}

		vk::DeviceCreateInfo device_create_info(
			vk::DeviceCreateFlags(),
			static_cast<uint32_t>( queue_create_infos.size() ),
			queue_create_infos.data(),
			// Device enabled layer count and enabled layer names are deprecated in newer version of Vulkan.
			// They will be ignored.
			ENABLE_VALIDATION_LAYERS ? static_cast<uint32_t>( VALIDATION_LAYERS.size() ) : 0,
			ENABLE_VALIDATION_LAYERS ? VALIDATION_LAYERS.data() : nullptr,
			//
			static_cast<uint32_t>( DEVICE_EXTENSIONS.size() ), 
			DEVICE_EXTENSIONS.data(),
			{},
			&physical_feature
		);

		return physical_device.createDevice( device_create_info );
	}
}


//==============================================================================
// Implementation of VulkanContext
//==============================================================================

VulkanContext::VulkanContext( SDL_Window& window )
{
	instance = create_vulkan_instance( raii_context, window );

	if( ENABLE_VALIDATION_LAYERS )
	{
		debug_messenger = instance.createDebugUtilsMessengerEXT( make_debug_info() );
		LOG( "Vulkan debug messenger is set up." );
	}
	else
	{
		LOG( "Vulkan debug messenger is not available." );
	}

	surface = create_vulkan_surface( instance, window );

	physical_device = create_physical_device( instance, surface, queue_indices );

	logical_device = create_logical_device( physical_device, queue_indices );

	// Retrieve the queue
	graphics_queue = logical_device.getQueue( queue_indices.graphics_family.value(), 0 );
	present_queue  = logical_device.getQueue( queue_indices.present_family.value(), 0 );
}

VulkanContext::~VulkanContext()
{
}