#include "ImGUILifetime.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_sdl2.h>

#include <utility/Logger.h>

#include <graphics/VulkanContext.h>

using namespace graphics;
using namespace utility;

ImGUILifetime::ImGUILifetime( VulkanContext& context )
	: mContext( context )
{
}

ImGUILifetime::~ImGUILifetime()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

bool
ImGUILifetime::Init( SDL_Window& window, uint32_t min_image_count, uint32_t max_image_count )
{
	LOG( "Initializing IMGUI ..." );

	const uint32_t max_sets = 1000;
	vk::DescriptorPoolSize pool_sizes[] =
	{
		{ vk::DescriptorType::eSampler, max_sets },
		{ vk::DescriptorType::eCombinedImageSampler, max_sets },
		{ vk::DescriptorType::eSampledImage, max_sets },
		{ vk::DescriptorType::eStorageImage, max_sets },
		{ vk::DescriptorType::eUniformTexelBuffer, max_sets },
		{ vk::DescriptorType::eStorageTexelBuffer, max_sets },
		{ vk::DescriptorType::eUniformBuffer, max_sets },
		{ vk::DescriptorType::eStorageBuffer, max_sets },
		{ vk::DescriptorType::eUniformBufferDynamic, max_sets },
		{ vk::DescriptorType::eStorageBufferDynamic, max_sets },
		{ vk::DescriptorType::eInputAttachment, max_sets }
	};

	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.poolSizeCount = static_cast<uint32_t>( std::size( pool_sizes ) );
	pool_info.pPoolSizes = pool_sizes;
	pool_info.maxSets = max_sets;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

	mDescriptorPool = mContext.logical_device->createDescriptorPoolUnique( pool_info );

	ImGui::CreateContext();

	if( !ImGui_ImplSDL2_InitForVulkan( &window ) )
	{
		LOG_ERROR( "Failed to initialize IMGUI for SDL2" );
		return false;
	}

	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = mContext.instance.get();
	init_info.PhysicalDevice = mContext.physical_device;
	init_info.Device = mContext.logical_device.get();
	init_info.Queue = mContext.graphics_queue;
	init_info.DescriptorPool = mDescriptorPool.get();
	init_info.MinImageCount = min_image_count;
	init_info.ImageCount = max_image_count;
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineRenderingCreateInfo = std::move( pipeline_rendering_create_info );
	
	
	if( !ImGui_ImplVulkan_Init( &init_info ) )
	{
		LOG_ERROR( "Failed to initialize IMGUI for Vulkan." );
		return false;
	}

	return true;
}