#ifndef _VULKAN_DESCRIPTOR_H_
#define _VULKAN_DESCRIPTOR_H_

#include <vulkan/vulkan.hpp>
#include <deque>
#include <unordered_map>

namespace eage::graphics
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
		DynamicDescriptorAllocator( vk::Device device, uint32_t sets_per_pool, const std::vector<DynamicDescriptorAllocator::PoolSizeRatio>& pool_ratios );
		~DynamicDescriptorAllocator();

		void Reset();

		vk::UniqueDescriptorSet Allocate( vk::DescriptorSetLayout layout );

	private:
		vk::UniqueDescriptorPool GetPool();
		vk::UniqueDescriptorPool CreatePool( uint32_t set_counts, const std::vector<DynamicDescriptorAllocator::PoolSizeRatio>& pool_sizes);
		void GrowSetsPerPool();

		vk::Device mDevice;
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

	/// AbstractUniformDescriptor: Base class for different types of uniform descriptors
	class AbstractUniformDescriptor
	{
	public:
		virtual ~AbstractUniformDescriptor() = default;

		/// 
		/// Check if the descriptor is dynamic (per-frame) or static (single)
		///
		virtual bool IsDynamic() const = 0;

		///
		/// Get the descriptor set for the current frame
		/// This could be a single descriptor set (for static descriptors) or per-frame descriptor sets (for dynamic descriptors)
		///
		virtual vk::DescriptorSet* GetDescriptorSet( size_t current_frame_index = 0 ) = 0;

		///
		/// Get the dynamic offsets for the current frame
		/// This is only relevant for dynamic descriptors
		///
		virtual std::vector<uint32_t>* GetDynamicOffsets( size_t current_frame_index = 0 ) = 0;

		///
		/// Write buffer to the descriptor set
		///
		virtual void WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize size ) = 0;

		///
		/// Write the image buffer to the descriptor set
		///
		virtual void WriteImage( uint32_t binding, vk::DescriptorType type, vk::ImageView image_view, vk::ImageLayout layout, vk::Sampler sampler ) = 0;
	};

	/// StaticDescriptor: For resources that don't change between frames (textures, static uniforms)
	/// - Allocates only one descriptor set
	/// - More memory efficient for static resources
	/// - Use for: material textures, constant material properties
	class StaticDescriptor final : public AbstractUniformDescriptor
	{
	public:
		StaticDescriptor( Renderer& renderer, vk::DescriptorSetLayout layout );
		
		// Implementation of AbstractUniformDescriptor
		virtual bool IsDynamic() const override { return false; }
		virtual vk::DescriptorSet* GetDescriptorSet( size_t frame_index = 0 ) override { return &mDescriptorSet.get(); };
		virtual std::vector<uint32_t>* GetDynamicOffsets( size_t frame_index = 0 ) override { return nullptr; }
		virtual void WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize size ) override;
		virtual void WriteImage( uint32_t binding, vk::DescriptorType type, vk::ImageView image_view, vk::ImageLayout layout, vk::Sampler sampler ) override;
		// End of implementation of AbstractUniformDescriptor

	private:
		Renderer& mRenderer;
		vk::UniqueDescriptorSet mDescriptorSet;
		DescriptorWriter mWriter;
	};

	/// DynamicDescriptor: For resources that change between frames (scene globals, per-object transforms)
	/// - Allocates per-frame descriptor sets
	/// - Supports dynamic uniform buffer offsets
	/// - Use for: view/projection matrices, model matrices, animation data
	class DynamicDescriptor final : public AbstractUniformDescriptor
	{
	public:
		DynamicDescriptor( Renderer& renderer, vk::DescriptorSetLayout layout );
		
		// Implementation of AbstractUniformDescriptor
		virtual bool IsDynamic() const override { return true; }
		virtual vk::DescriptorSet* GetDescriptorSet( size_t frame_index ) override;
		virtual std::vector<uint32_t>* GetDynamicOffsets( size_t frame_index ) override;
		virtual void WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize size ) override;
		virtual void WriteImage( uint32_t binding, vk::DescriptorType type, vk::ImageView image_view, vk::ImageLayout layout, vk::Sampler sampler ) override;
		// End of implementation of AbstractUniformDescriptor

	private:
		Renderer& mRenderer;
		struct PerFrameState
		{
			vk::UniqueDescriptorSet descriptor_set;
			std::vector<uint32_t> dynamic_offsets;
		};
		std::unordered_map<size_t, PerFrameState> mPerFrameDescriptors;
		DescriptorWriter mWriter;
	};
}

#endif // _VULKAN_DESCRIPTOR_H_