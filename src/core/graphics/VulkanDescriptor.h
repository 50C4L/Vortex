#ifndef _VULKAN_DESCRIPTOR_H_
#define _VULKAN_DESCRIPTOR_H_

#include <vulkan/vulkan.hpp>

namespace graphics
{
	class DescriptorLayoutBuilder
	{
	public:
		DescriptorLayoutBuilder();
		~DescriptorLayoutBuilder();

		DescriptorLayoutBuilder& AddBinding( uint32_t binding, vk::DescriptorType type );
		vk::UniqueDescriptorSetLayout Build( vk::Device device, vk::ShaderStageFlags shader_stage );
		void Clear();

	private:
		std::vector<vk::DescriptorSetLayoutBinding> mBindings;
	};

	class DescriptorAllocator
	{
	public:
		struct PoolSizeRatio
		{
			vk::DescriptorType type;
			float ratio;
		};

		DescriptorAllocator( vk::Device& device, uint32_t max_sets, const std::vector<PoolSizeRatio>& pool_sizes );
		~DescriptorAllocator();

		void Reset();

		vk::UniqueDescriptorSet Allocate( vk::DescriptorSetLayout layout );

	private:
		vk::Device& mDevice;
		vk::UniqueDescriptorPool mPool;
	};
}

#endif // _VULKAN_DESCRIPTOR_H_