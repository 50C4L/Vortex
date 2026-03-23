#ifndef _EAGE_SHADER_REFLECTION_H_
#define _EAGE_SHADER_REFLECTION_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace eage::graphics
{
	struct DescriptorBindingInfo
	{
		uint32_t set;
		uint32_t binding;
		vk::DescriptorType descriptor_type;
		uint32_t count;
		std::string name;
	};

	struct ShaderReflectionData
	{
		std::vector<DescriptorBindingInfo> bindings;
	};

	/// Reflects a single SPIR-V shader stage and returns its descriptor binding metadata.
	ShaderReflectionData reflect_shader( const std::vector<uint32_t>& spirv_code );

	/// Merges reflection data from two shader stages (e.g. vertex + fragment).
	/// Deduplicates bindings that appear in both stages.
	ShaderReflectionData merge_reflection( const ShaderReflectionData& a, const ShaderReflectionData& b );

	/// Builds one vk::UniqueDescriptorSetLayout per descriptor set found in the reflection data.
	/// The returned map is keyed by set number.
	/// Sets listed in skip_sets are excluded from the result.
	std::map<uint32_t, vk::UniqueDescriptorSetLayout> build_descriptor_set_layouts(
		vk::Device device,
		const ShaderReflectionData& reflection,
		vk::ShaderStageFlags stage_flags,
		const std::vector<uint32_t>& skip_sets = {} );
}

#endif // _EAGE_SHADER_REFLECTION_H_
