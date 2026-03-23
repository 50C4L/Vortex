#include "ShaderReflection.h"

#include <spirv-reflect/spirv_reflect.h>
#include <graphics/VulkanDescriptor.h>
#include <utility/Logger.h>

#include <algorithm>
#include <set>

using namespace eage::graphics;
using namespace utility;

namespace
{
	vk::DescriptorType spv_to_vk_descriptor_type( SpvReflectDescriptorType spv_type )
	{
		switch( spv_type )
		{
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:                return vk::DescriptorType::eSampler;
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return vk::DescriptorType::eCombinedImageSampler;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:          return vk::DescriptorType::eSampledImage;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:          return vk::DescriptorType::eStorageImage;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:   return vk::DescriptorType::eUniformTexelBuffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:   return vk::DescriptorType::eStorageTexelBuffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:         return vk::DescriptorType::eUniformBuffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:         return vk::DescriptorType::eStorageBuffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return vk::DescriptorType::eUniformBufferDynamic;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return vk::DescriptorType::eStorageBufferDynamic;
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:       return vk::DescriptorType::eInputAttachment;
			default:                                                 return vk::DescriptorType::eUniformBuffer;
		}
	}
}

ShaderReflectionData
eage::graphics::reflect_shader( const std::vector<uint32_t>& spirv_code )
{
	ShaderReflectionData result;

	SpvReflectShaderModule spv_module;
	SpvReflectResult spv_result = spvReflectCreateShaderModule(
		spirv_code.size() * sizeof( uint32_t ),
		spirv_code.data(),
		&spv_module );

	if( spv_result != SPV_REFLECT_RESULT_SUCCESS )
	{
		LOG_ERROR( "SPIRV-Reflect: failed to create shader module" );
		return result;
	}

	uint32_t binding_count = 0;
	spvReflectEnumerateDescriptorBindings( &spv_module, &binding_count, nullptr );

	std::vector<SpvReflectDescriptorBinding*> spv_bindings( binding_count );
	spvReflectEnumerateDescriptorBindings( &spv_module, &binding_count, spv_bindings.data() );

	for( uint32_t i = 0; i < binding_count; ++i )
	{
		const auto* b = spv_bindings[i];

		DescriptorBindingInfo info;
		info.set             = b->set;
		info.binding         = b->binding;
		info.descriptor_type = spv_to_vk_descriptor_type( b->descriptor_type );
		info.count           = b->count;
		info.name            = b->name ? b->name : "";

		result.bindings.push_back( std::move( info ) );
	}

	spvReflectDestroyShaderModule( &spv_module );

	return result;
}

ShaderReflectionData
eage::graphics::merge_reflection( const ShaderReflectionData& a, const ShaderReflectionData& b )
{
	ShaderReflectionData merged;
	merged.bindings = a.bindings;

	// Track existing (set, binding) pairs to avoid duplicates.
	std::set<std::pair<uint32_t, uint32_t>> existing;
	for( const auto& binding : a.bindings )
	{
		existing.insert( { binding.set, binding.binding } );
	}

	for( const auto& binding : b.bindings )
	{
		if( existing.find( { binding.set, binding.binding } ) == existing.end() )
		{
			merged.bindings.push_back( binding );
			existing.insert( { binding.set, binding.binding } );
		}
	}

	return merged;
}

std::map<uint32_t, vk::UniqueDescriptorSetLayout>
eage::graphics::build_descriptor_set_layouts(
	vk::Device device,
	const ShaderReflectionData& reflection,
	vk::ShaderStageFlags stage_flags,
	const std::vector<uint32_t>& skip_sets )
{
	std::set<uint32_t> skip( skip_sets.begin(), skip_sets.end() );

	// Group bindings by set number.
	std::map<uint32_t, std::vector<const DescriptorBindingInfo*>> sets;
	for( const auto& binding : reflection.bindings )
	{
		if( skip.count( binding.set ) )
		{
			continue;
		}
		sets[binding.set].push_back( &binding );
	}

	std::map<uint32_t, vk::UniqueDescriptorSetLayout> layouts;
	for( const auto& [set_number, bindings] : sets )
	{
		DescriptorLayoutBuilder builder;
		for( const auto* b : bindings )
		{
			builder.AddBinding( b->binding, b->descriptor_type );
		}
		layouts[set_number] = builder.Build( device, stage_flags );
	}

	return layouts;
}
