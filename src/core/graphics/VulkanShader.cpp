#include "VulkanShader.h"

#include <fstream>

using namespace eage::graphics;

std::optional<vk::UniqueShaderModule> 
eage::graphics::create_shader_module_from_file( vk::Device& device, const std::string& spv_file_path )
{
	std::ifstream file( spv_file_path, std::ios::ate | std::ios::binary );

	if( !file.is_open() )
	{
		return std::nullopt;
	}

	size_t file_size = static_cast<size_t>( file.tellg() );
	std::vector<char> buffer( file_size );

	file.seekg( 0 );
	file.read( buffer.data(), file_size );

	file.close();

	vk::ShaderModuleCreateInfo create_info{};
	create_info.codeSize = buffer.size() ;
	create_info.pCode    = reinterpret_cast<const uint32_t*>( buffer.data() );

	return device.createShaderModuleUnique( create_info );
}