#ifndef _VULKAN_SHADER_H_
#define _VULKAN_SHADER_H_

#include <vulkan/vulkan.hpp>
#include <optional>

namespace eage::graphics
{
	std::optional<vk::UniqueShaderModule> create_shader_module_from_file( vk::Device& device, const std::string& spv_file_path );
}

#endif // _VULKAN_SHADER_H_