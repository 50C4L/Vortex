#include "MaterialBuilder.h"

using namespace eage::graphics;

MaterialBuilder& MaterialBuilder::SetVertexShader( const std::string& path )
{
	mProperty.vertex_shader_path = path;
	return *this;
}

MaterialBuilder& MaterialBuilder::SetFragmentShader( const std::string& path )
{
	mProperty.fragment_shader_path = path;
	return *this;
}

MaterialBuilder& MaterialBuilder::SetShaders( const std::string& vertex_path, const std::string& fragment_path )
{
	mProperty.vertex_shader_path = vertex_path;
	mProperty.fragment_shader_path = fragment_path;
	return *this;
}

MaterialBuilder& MaterialBuilder::AddTexture( const std::string& texture_path )
{
	TextureBinding texture_binding;
	texture_binding.texture_path = texture_path;
	texture_binding.min_filter = TextureFilter::NEAREST;
	texture_binding.mag_filter = TextureFilter::NEAREST;
	texture_binding.address_mode = AddressMode::REPEAT;
	
	mProperty.textures.push_back( texture_binding );
	return *this;
}

MaterialBuilder& MaterialBuilder::AddTexture( const std::string& texture_path,
												TextureFilter min_filter, TextureFilter mag_filter )
{
	TextureBinding texture_binding;
	texture_binding.texture_path = texture_path;
	texture_binding.min_filter = min_filter;
	texture_binding.mag_filter = mag_filter;
	texture_binding.address_mode = AddressMode::REPEAT;
	
	mProperty.textures.push_back( texture_binding );
	return *this;
}

MaterialBuilder& MaterialBuilder::AddTexture( const std::string& name, const std::string& texture_path,
												TextureFilter min_filter, TextureFilter mag_filter )
{
	TextureBinding texture_binding;
	texture_binding.name = name;
	texture_binding.texture_path = texture_path;
	texture_binding.min_filter = min_filter;
	texture_binding.mag_filter = mag_filter;
	texture_binding.address_mode = AddressMode::REPEAT;
	
	mProperty.textures.push_back( texture_binding );
	return *this;
}

MaterialBuilder& MaterialBuilder::SetTopology( Topology topology )
{
	mProperty.topology = topology;
	return *this;
}

MaterialBuilder& MaterialBuilder::SetAlphaBlending()
{
	mProperty.blend_enable = true;
	mProperty.src_color_blend = BlendFactor::SRC_ALPHA;
	mProperty.dst_color_blend = BlendFactor::ONE_MINUS_SRC_ALPHA;
	mProperty.color_blend_op = BlendOp::ADD;
	return *this;
}

MaterialBuilder& MaterialBuilder::SetAdditiveBlending()
{
	mProperty.blend_enable = true;
	mProperty.src_color_blend = BlendFactor::ONE;
	mProperty.dst_color_blend = BlendFactor::ONE;
	mProperty.color_blend_op = BlendOp::ADD;
	return *this;
}

MaterialBuilder& MaterialBuilder::EnableDepthTest( bool enable )
{
	mProperty.depth_test = enable;
	return *this;
}

MaterialProperty MaterialBuilder::Build() const
{
	return mProperty;
}