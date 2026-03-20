#ifndef _EAGE_GRAPHICS_MATERIAL_BUILDER_H_
#define _EAGE_GRAPHICS_MATERIAL_BUILDER_H_

#include <graphics/MaterialProperty.h>

namespace eage::graphics
{
	class MaterialBuilder
	{
	public:
		MaterialBuilder() = default;
		
		// Shader methods
		MaterialBuilder& SetVertexShader( const std::string& path );
		MaterialBuilder& SetFragmentShader( const std::string& path );
		MaterialBuilder& SetShaders( const std::string& vertex_path, const std::string& fragment_path );
		
		// Texture methods
		MaterialBuilder& AddTexture( uint32_t binding, const std::string& texture_path );
		MaterialBuilder& AddTexture( uint32_t binding, const std::string& texture_path, 
									 TextureFilter min_filter, TextureFilter mag_filter );
		
		// Pipeline state methods
		MaterialBuilder& SetTopology( Topology topology );
		MaterialBuilder& SetAlphaBlending();
		MaterialBuilder& SetAdditiveBlending();
		MaterialBuilder& EnableDepthTest( bool enable = true );
		
		// Build method returns the property, not ResourceId
		MaterialProperty Build() const;
		
	private:
		MaterialProperty mProperty;
	};
}

#endif // _EAGE_GRAPHICS_MATERIAL_BUILDER_H_