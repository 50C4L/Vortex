#ifndef _VULKAN_DESCRIPTOR_H_
#define _VULKAN_DESCRIPTOR_H_

#include <vulkan/vulkan.hpp>
#include <deque>
#include <unordered_map>

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

	class DescriptorWriter
	{
	public:
		DescriptorWriter() = default;
		~DescriptorWriter() = default;

		void WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range );
		void WriteImage( uint32_t binding, vk::DescriptorType type, vk::ImageView image_view, vk::ImageLayout layout, vk::Sampler sampler );

		void Update( vk::Device device, vk::DescriptorSet descriptor_set );
		void AddImageInfo( vk::DescriptorImageInfo image_info );
		void AddBufferInfo( vk::DescriptorBufferInfo buffer_info );
		void Clear();

	private:
		std::vector<vk::WriteDescriptorSet> mWrites;
		std::deque<vk::DescriptorImageInfo> mImageInfos;
		std::deque<vk::DescriptorBufferInfo> mBufferInfos;
	};

	class Renderer;

	class UniformDescriptor
	{
	public:
		UniformDescriptor( Renderer& renderer, vk::DescriptorSetLayout layout );
		~UniformDescriptor();

		vk::DescriptorSet GetDescriptorSet( size_t current_frame_index ) const;

		void WriteBuffer( size_t current_frame_index, uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range );

	private:
		Renderer& mRenderer;
		DescriptorWriter mWriter;
		std::unordered_map<size_t, vk::UniqueDescriptorSet> mPerFrameDescriptorSets;
	};
}

#endif // _VULKAN_DESCRIPTOR_H_