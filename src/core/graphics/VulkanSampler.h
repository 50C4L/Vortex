#ifndef VULKAN_SAMPLER_H
#define VULKAN_SAMPLER_H

#include <vulkan/vulkan.hpp>

namespace graphics
{
	class VulkanSampler
	{
	public:
		VulkanSampler( vk::Device& device, vk::SamplerCreateInfo& create_info );
		virtual ~VulkanSampler();

		vk::Sampler& GetSampler();

	private:
		vk::UniqueSampler mSampler;
	};
}

#endif // VULKAN_SAMPLER_H