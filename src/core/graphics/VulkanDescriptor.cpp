#include "VulkanDescriptor.h"

using namespace graphics;

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

DescriptorAllocator::DescriptorAllocator( vk::Device& device, uint32_t max_sets, const std::vector<PoolSizeRatio>& pool_sizes )
	: mDevice( device )
{
	std::vector<vk::DescriptorPoolSize> pool_sizes_vk;
	for( const auto& pool_size : pool_sizes )
	{
		pool_sizes_vk.push_back( vk::DescriptorPoolSize( pool_size.type, static_cast<uint32_t>( max_sets * pool_size.ratio ) ) );
	}

	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.poolSizeCount = static_cast<uint32_t>( pool_sizes_vk.size() );
	pool_info.pPoolSizes    = pool_sizes_vk.data();
	pool_info.maxSets       = max_sets;

	mPool = mDevice.createDescriptorPoolUnique( pool_info );
}

DescriptorAllocator::~DescriptorAllocator()
{
}

void
DescriptorAllocator::Reset()
{
	mDevice.resetDescriptorPool( mPool.get() );
}

vk::UniqueDescriptorSet
DescriptorAllocator::Allocate( vk::DescriptorSetLayout layout )
{
	vk::DescriptorSetAllocateInfo alloc_info;
	alloc_info.descriptorPool     = mPool.get();
	alloc_info.descriptorSetCount  = 1;
	alloc_info.pSetLayouts         = &layout;

	return std::move( mDevice.allocateDescriptorSetsUnique( alloc_info )[ 0 ] );
}