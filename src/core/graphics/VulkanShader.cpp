#include "VulkanShader.h"

#include <fstream>

using namespace eage::graphics;

std::optional<ShaderAsset>
eage::graphics::load_shader_from_file( vk::Device device, const std::string& spv_file_path )
{
	std::ifstream file( spv_file_path, std::ios::ate | std::ios::binary );

	if( !file.is_open() )
	{
		return std::nullopt;
	}

	size_t file_size = static_cast<size_t>( file.tellg() );
	std::vector<uint32_t> spirv_code( file_size / sizeof( uint32_t ) );

	file.seekg( 0 );
	file.read( reinterpret_cast<char*>( spirv_code.data() ), file_size );
	file.close();

	vk::ShaderModuleCreateInfo create_info{};
	create_info.codeSize = file_size;
	create_info.pCode    = spirv_code.data();

	ShaderAsset asset;
	asset.module    = device.createShaderModuleUnique( create_info );
	asset.spirv_code = std::move( spirv_code );

	return asset;
}

std::optional<vk::UniqueShaderModule>
eage::graphics::create_shader_module_from_file( vk::Device device, const std::string& spv_file_path )
{
	auto asset = load_shader_from_file( device, spv_file_path );
	if( !asset )
	{
		return std::nullopt;
	}
	return std::move( asset->module );
}