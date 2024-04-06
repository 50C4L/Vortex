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

	class DynamicDescriptorAllocator
	{
	public:
		struct PoolSizeRatio
		{
			vk::DescriptorType type;
			float ratio;
		};
		DynamicDescriptorAllocator( vk::Device& device, uint32_t sets_per_pool, const std::vector<DynamicDescriptorAllocator::PoolSizeRatio>& pool_ratios );
		~DynamicDescriptorAllocator();

		void Reset();

		vk::UniqueDescriptorSet Allocate( vk::DescriptorSetLayout layout );

	private:
		vk::UniqueDescriptorPool GetPool();
		vk::UniqueDescriptorPool CreatePool( uint32_t set_counts, const std::vector<DynamicDescriptorAllocator::PoolSizeRatio>& pool_sizes);
		void GrowSetsPerPool();

		vk::Device& mDevice;
		std::vector<DynamicDescriptorAllocator::PoolSizeRatio> mRatios;
		std::vector<vk::UniqueDescriptorPool> mFullPools;
		std::vector<vk::UniqueDescriptorPool> mFreePools;
		uint32_t mSetsPerPool;
	};
}

#endif // _VULKAN_DESCRIPTOR_H_