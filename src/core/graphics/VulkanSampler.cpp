#include "VulkanSampler.h"

using namespace graphics;

VulkanSampler::VulkanSampler( vk::Device& device, vk::SamplerCreateInfo& create_info )
{
	mSampler = device.createSamplerUnique( create_info );
}

VulkanSampler::~VulkanSampler()
{
}

vk::Sampler& VulkanSampler::GetSampler()
{
	return mSampler.get();
}