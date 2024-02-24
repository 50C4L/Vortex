#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>

#include <utility/Logger.h>

using namespace graphics;
using namespace utility;

namespace
{
#ifdef NDEBUG
	const bool ENABLE_VALIDATION_LAYERS = false;
#else
	const bool ENABLE_VALIDATION_LAYERS = true;
#endif

	const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

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
	/// @param message_severity
	///  Severity of this debug info, can be one of the followings
	///   - VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
	///   - VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
	///   - VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
	///   - VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
	/// 
	/// @param message_type
	///  Type of the debug info, can be one of the followings
	///   - VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
	///   - VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	///   - VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
	/// 
	/// @param pCallbackData
	///  Message data
	/// 
	/// @param pUserData
	///  Whatever you pass in
	/// 
	VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data )
	{
		const std::string msg = "Vulkan Validation | " + std::string{ callback_data->pMessage };
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
	/// Vulkan extention functions to create and destroy a `VkDebugUtilsMessengerEXT`
	/// TODO: It should be able to use instance.CreateDebugUtilsMessengerEXT( ... ) and instance.DestroyDebugUtilsMessengerEXT( ... )
	/// so we can get rid of this address retrieving shit.
	/// 
	VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger )
	{
		auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
		if( func != nullptr ) 
		{
			return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
		}
		else 
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator )
	{
		auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
		if( func != nullptr ) 
		{
			func( instance, callback, pAllocator );
		}
	}
}

namespace graphics
{
	// Simple wrapper for the vulkan debug messenger
	class VulkanDebugMessenger
	{
	public:
		VulkanDebugMessenger( vk::Instance& instance )
			: mInstance( instance )
			, mMessenger( nullptr )
		{
			auto debug_info = make_debug_info();
			if( CreateDebugUtilsMessengerEXT( mInstance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>( &debug_info ), nullptr, &mMessenger ) !=	VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to set up vulkan debug messenger!" );
			}
			LOG( "Vulkan debug messenger is set up." );
		}

		~VulkanDebugMessenger()
		{
			if( mMessenger )
			{
				DestroyDebugUtilsMessengerEXT( mInstance, mMessenger, nullptr );
			}
		}

	private:
		vk::Instance& mInstance;
		VkDebugUtilsMessengerEXT mMessenger;
	};
}

namespace
{
	vk::UniqueInstance create_vulkan_instance( SDL_Window& window )
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

		vk::UniqueInstance instance;
		try 
		{
			instance = vk::createInstanceUnique( std::move( create_info ) );
		}
		catch ( vk::SystemError /*err*/ )
		{
			throw std::runtime_error( "Failed to create vulkan instance!" );
		}
		LOG( "OK" );

		LOG( "Retrieving Vulkan extensions:" );
		auto ins_extensions = vk::enumerateInstanceExtensionProperties();

		for( const auto& extension : ins_extensions )
		{
			LOG( '\t' + extension.extensionName );
		}
		
		return instance; 
	}

	std::unique_ptr<VulkanDebugMessenger>
	setup_debug_messenger( vk::Instance& instance )
	{
		if( !ENABLE_VALIDATION_LAYERS )
		{
			return nullptr;
		}
		return std::make_unique<VulkanDebugMessenger>( instance );
	}

	vk::UniqueSurfaceKHR
	create_vulkan_surface( vk::Instance& instance, SDL_Window& window )
	{
		VkSurfaceKHR surface;
		if( !SDL_Vulkan_CreateSurface( &window, instance, &surface ) )
		{
			throw std::runtime_error( "Failed to create Vulkan surface!" );
		}
		return vk::UniqueSurfaceKHR( surface, vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic>( instance ) );
	}
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

bool
Renderer::Init( SDL_Window& window )
{
	mInstance = create_vulkan_instance( window );
	if( !mInstance )
	{
		LOG_ERROR( "Failed to create Vulkan instance" );
		return false;
	}

	mVulkanDebugMessenger = setup_debug_messenger( *mInstance );
	if( !mVulkanDebugMessenger )
	{
		LOG( "Vulkan debug messenger is not available." );
	}

	mSurface = create_vulkan_surface( *mInstance, window );
	if( !mSurface )
	{
		LOG_ERROR( "Failed to create Vulkan surface" );
		return false;
	}

	return true;
}

void
Renderer::Render()
{
}
