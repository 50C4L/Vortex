#ifndef _EAGE_GRAPHICS_MATERIAL_PROPERTY_H_
#define _EAGE_GRAPHICS_MATERIAL_PROPERTY_H_

#include <string>
#include <vector>
#include <graphics/GraphicsTypes.h>

namespace eage::graphics
{
	// Represents a texture binding for a material.
	// Textures are matched to shader bindings by name or declaration order.
	struct TextureBinding
	{
		/// Optional GLSL variable name used for name-based matching to the shader binding.
		/// If empty, textures are matched in declaration order.
		std::string name;
		/// File path to the texture image.
		std::string texture_path;
		/// Minification filter to use when sampling the texture.
		TextureFilter min_filter = TextureFilter::NEAREST;
		/// Magnification filter to use when sampling the texture.
		TextureFilter mag_filter = TextureFilter::NEAREST;
		/// Address mode for texture coordinates outside [0, 1] range.
		AddressMode address_mode = AddressMode::REPEAT;
	};

	// Represents a uniform binding for a material.
	// Uniforms are matched to shader bindings by name or declaration order.
	struct UniformBinding
	{
		/// Optional GLSL variable name used for name-based matching to the shader binding.
		std::string name;
		/// Size of the uniform data in bytes.
		size_t size;
		/// Pointer to the uniform data.
		void* data = nullptr;
	};

	// Describes all properties required to define a material, including shaders, textures, uniforms, and pipeline state.
	struct MaterialProperty
	{
		/// File path to the vertex shader.
		std::string vertex_shader_path;
		/// File path to the fragment shader.
		std::string fragment_shader_path;
		
		/// List of texture bindings used by the material.
		std::vector<TextureBinding> textures;
		/// List of uniform bindings used by the material.
		std::vector<UniformBinding> uniforms;
		
		// Pipeline state

		/// Primitive topology used for rendering (e.g., triangle list).
		Topology topology = Topology::TRIANGLE_LIST;
		/// Polygon mode (e.g., fill, line, point).
		PolygonMode polygon_mode = PolygonMode::FILL;
		/// Face culling mode.
		CullMode cull_mode = CullMode::NONE;
		/// Winding order for front-facing polygons.
		FrontFace front_face = FrontFace::CLOCKWISE;
		
		// Blending

		/// Enables or disables color blending.
		bool blend_enable = true;
		/// Source blend factor for color blending.
		BlendFactor src_color_blend = BlendFactor::ONE;
		/// Destination blend factor for color blending.
		BlendFactor dst_color_blend = BlendFactor::DST_ALPHA;
		/// Blend operation for color blending.
		BlendOp color_blend_op = BlendOp::ADD;
		
		// Depth testing

		/// Enables or disables depth testing.
		bool depth_test = true;
		/// Enables or disables writing to the depth buffer.
		bool depth_write = true;
		/// Depth comparison operation.
		CompareOp depth_compare = CompareOp::GREATER_OR_EQUAL;
	};
}

#endif // _EAGE_GRAPHICS_MATERIAL_PROPERTY_H_