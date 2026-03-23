#ifndef _VULKAN_SHADER_H_
#define _VULKAN_SHADER_H_

#include <vulkan/vulkan.hpp>
#include <optional>
#include <vector>

namespace eage::graphics
{
	struct ShaderAsset
	{
		vk::UniqueShaderModule module;
		std::vector<uint32_t> spirv_code;
	};

	std::optional<ShaderAsset> load_shader_from_file( vk::Device& device, const std::string& spv_file_path );

	/// Legacy wrapper - returns only the shader module without SPIR-V bytecode.
	std::optional<vk::UniqueShaderModule> create_shader_module_from_file( vk::Device& device, const std::string& spv_file_path );
}

#endif // _VULKAN_SHADER_H_