#include "VulkanDescriptor.h"

#include <graphics/Renderer.h>
#include <utility/Logger.h>

using namespace eage::graphics;
using namespace utility;

namespace
{
	const float SETS_PER_POOL_GROWTH = 1.5f;
	const uint32_t MAX_SETS_PER_POOL = 2048;
}

DescriptorLayoutBuilder::DescriptorLayoutBuilder()
{
}

DescriptorLayoutBuilder::~DescriptorLayoutBuilder()
{
}

DescriptorLayoutBuilder&
DescriptorLayoutBuilder::AddBinding( uint32_t binding, vk::DescriptorType type )
{
	mBindings.push_back( vk::DescriptorSetLayoutBinding( binding, type, 1 ) );
	return *this;
}

vk::UniqueDescriptorSetLayout
DescriptorLayoutBuilder::Build( vk::Device device, vk::ShaderStageFlags shader_stage )
{
	for( auto& binding : mBindings )
	{
		binding.stageFlags |= shader_stage;
	}

	vk::DescriptorSetLayoutCreateInfo layout_info;
	layout_info.bindingCount = static_cast<uint32_t>( mBindings.size() );
	layout_info.pBindings    = mBindings.data();

	return device.createDescriptorSetLayoutUnique( layout_info );
}

void
DescriptorLayoutBuilder::Clear()
{
	mBindings.clear();
}

DynamicDescriptorAllocator::DynamicDescriptorAllocator( vk::Device device, uint32_t sets_per_pool, const std::vector<PoolSizeRatio>& pool_sizes )
	: mDevice( device )
	, mRatios( pool_sizes )
	, mSetsPerPool( sets_per_pool )
{
	mFreePools.push_back( std::move( CreatePool( sets_per_pool, pool_sizes ) ) );
	GrowSetsPerPool();
}

DynamicDescriptorAllocator::~DynamicDescriptorAllocator()
{
}

void
DynamicDescriptorAllocator::Reset()
{
	for( auto& pool : mFreePools )
	{
		mDevice.resetDescriptorPool( pool.get() );
	}

	for( auto& pool : mFullPools )
	{
		mDevice.resetDescriptorPool( pool.get() );
		mFreePools.push_back( std::move( pool ) );
	}
	mFullPools.clear();
}

vk::UniqueDescriptorSet
DynamicDescriptorAllocator::Allocate( vk::DescriptorSetLayout layout )
{
	auto pool_to_use = GetPool();

	vk::DescriptorSetAllocateInfo alloc_info;
	alloc_info.descriptorPool		= pool_to_use.get();
	alloc_info.descriptorSetCount	= 1;
	alloc_info.pSetLayouts			= &layout;

	vk::UniqueDescriptorSet ret;

	try
	{
		ret = std::move( mDevice.allocateDescriptorSetsUnique( alloc_info )[0] );
	}
	catch( const vk::SystemError& e )
	{
		if( e.code().value() == VK_ERROR_FRAGMENTED_POOL || e.code().value() == VK_ERROR_OUT_OF_POOL_MEMORY )
		{
			mFullPools.push_back( std::move( pool_to_use ) );
			pool_to_use = GetPool();
			alloc_info.descriptorPool = pool_to_use.get();
			ret = std::move( mDevice.allocateDescriptorSetsUnique( alloc_info )[0] );
		}
		else
		{
			throw e;
		}
	}
	
	mFreePools.push_back( std::move( pool_to_use ) );
	return std::move( ret );
}

vk::UniqueDescriptorPool
DynamicDescriptorAllocator::GetPool()
{
	vk::UniqueDescriptorPool ret;
	if( !mFreePools.empty() )
	{
		ret = std::move( mFreePools.back() );
		mFreePools.pop_back();
	}
	else
	{
		ret = CreatePool( mSetsPerPool, mRatios );
		GrowSetsPerPool();
	}

	return ret;
}

vk::UniqueDescriptorPool
DynamicDescriptorAllocator::CreatePool( uint32_t set_counts, const std::vector<PoolSizeRatio>& pool_ratios )
{
	std::vector<vk::DescriptorPoolSize> pool_sizes_vk;
	for( const auto& ratio : pool_ratios )
	{
		pool_sizes_vk.push_back( 
			vk::DescriptorPoolSize( ratio.type, static_cast<uint32_t>( set_counts * ratio.ratio ) ) );
	}

	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.poolSizeCount	= static_cast<uint32_t>( pool_sizes_vk.size() );
	pool_info.pPoolSizes	= pool_sizes_vk.data();
	pool_info.maxSets		= set_counts;
	pool_info.flags			= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

	return mDevice.createDescriptorPoolUnique( pool_info );
}

void
DynamicDescriptorAllocator::GrowSetsPerPool()
{
	mSetsPerPool = static_cast<uint32_t>( mSetsPerPool * SETS_PER_POOL_GROWTH );
	if( mSetsPerPool > MAX_SETS_PER_POOL )
	{
		mSetsPerPool = MAX_SETS_PER_POOL;
	}
}

void
DescriptorWriter::WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range )
{
	vk::DescriptorBufferInfo& buffer_info = mBufferInfos.emplace_back( buffer, offset, range );
	vk::WriteDescriptorSet write{};
	write.dstBinding = binding;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &buffer_info;
	mWrites.push_back( std::move( write ) );
}

void
DescriptorWriter::WriteImage( uint32_t binding, vk::DescriptorType type, vk::ImageView image_view, vk::ImageLayout layout, vk::Sampler sampler )
{
	vk::DescriptorImageInfo& image_info = mImageInfos.emplace_back( sampler, image_view, layout );
	vk::WriteDescriptorSet write{};
	write.dstBinding = binding;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &image_info;
	mWrites.push_back( std::move( write ) );
}

void
DescriptorWriter::Update( vk::Device device, vk::DescriptorSet descriptor_set )
{
	for( auto& write : mWrites )
	{
		write.dstSet = descriptor_set;
	}
	device.updateDescriptorSets( static_cast<uint32_t>( mWrites.size() ), mWrites.data(), 0, nullptr );
}

void
DescriptorWriter::Clear()
{
	mWrites.clear();
	mImageInfos.clear();
	mBufferInfos.clear();
}

void
DescriptorWriter::AddImageInfo( vk::DescriptorImageInfo image_info )
{
	mImageInfos.push_back( image_info );
}

void
DescriptorWriter::AddBufferInfo( vk::DescriptorBufferInfo buffer_info )
{
	mBufferInfos.push_back( buffer_info );
}

// StaticDescriptor Implementation
StaticDescriptor::StaticDescriptor( Renderer& renderer, vk::DescriptorSetLayout layout )
	: mRenderer( renderer )
{
	// Allocate a single descriptor set (not per-frame)
	auto& frame = mRenderer.GetFrames()[0]; // Use any frame's allocator
	mDescriptorSet = frame.descriptor_allocator->Allocate( layout );
}

void
StaticDescriptor::WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize range )
{
	mWriter.WriteBuffer( binding, type, buffer, 0, range );
	mWriter.Update( mRenderer.GetDevice(), mDescriptorSet.get() );
	mWriter.Clear();
}

void
StaticDescriptor::WriteImage( uint32_t binding, vk::DescriptorType type, vk::ImageView image_view, vk::ImageLayout layout, vk::Sampler sampler )
{
	mWriter.WriteImage( binding, type, image_view, layout, sampler );
	mWriter.Update( mRenderer.GetDevice(), mDescriptorSet.get() );
	mWriter.Clear();
}

// DynamicDescriptor Implementation
DynamicDescriptor::DynamicDescriptor( Renderer& renderer, vk::DescriptorSetLayout layout )
	: mRenderer( renderer )
{
	auto& frames = mRenderer.GetFrames();
	for( size_t i = 0; i < frames.size(); ++i )
	{
		auto descriptor_set = frames[i].descriptor_allocator->Allocate( layout );
		mPerFrameDescriptors[i] = { std::move( descriptor_set ), {} };
	}
}

void
DynamicDescriptor::WriteBuffer( uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, vk::DeviceSize sub_size )
{
	auto& frames = mRenderer.GetFrames();
	for( size_t i = 0; i < frames.size(); ++i )
	{
		mPerFrameDescriptors[i].dynamic_offsets.push_back( static_cast<uint32_t>( i * sub_size ) );
		mWriter.WriteBuffer( binding, type, buffer, 0, sub_size );
		mWriter.Update( mRenderer.GetDevice(), mPerFrameDescriptors[i].descriptor_set.get() );
		mWriter.Clear();
	}
}

vk::DescriptorSet*
DynamicDescriptor::GetDescriptorSet( size_t frame_index )
{
	auto find = mPerFrameDescriptors.find( frame_index );
	if( find == mPerFrameDescriptors.end() )
	{
		LOG_ERROR( "Descriptor set not found for the current frame index" );
		throw std::runtime_error( "Descriptor set not found for the current frame index" );
	}
	return &find->second.descriptor_set.get();
}

std::vector<uint32_t>*
DynamicDescriptor::GetDynamicOffsets( size_t frame_index )
{
	auto find = mPerFrameDescriptors.find( frame_index );
	if( find == mPerFrameDescriptors.end() )
	{
		LOG_ERROR( "Descriptor set not found for the current frame index" );
		throw std::runtime_error( "Descriptor set not found for the current frame index" );
	}
	return &find->second.dynamic_offsets;
}

void 
DynamicDescriptor::WriteImage( uint32_t /* binding */, vk::DescriptorType /* type */, vk::ImageView /* image_view */, 
		vk::ImageLayout /* layout */, vk::Sampler /* sampler */ )
{
	EAGE_ASSERT( false, "DynamicDescriptor does not support image descriptors." );
}
