#ifndef _EAGE_GRAPHICS_MATERIAL_PROPERTY_H_
#define _EAGE_GRAPHICS_MATERIAL_PROPERTY_H_

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace eage::graphics
{
	// Represents a texture binding for a material, including binding index, texture path, and sampler settings.
	struct TextureBinding
	{
		/// Descriptor set binding index for the texture.
		uint32_t binding;
		/// File path to the texture image.
		std::string texture_path;
		/// Minification filter to use when sampling the texture.
		vk::Filter min_filter = vk::Filter::eNearest;
		/// Magnification filter to use when sampling the texture.
		vk::Filter mag_filter = vk::Filter::eNearest;
		/// Address mode for texture coordinates outside [0, 1] range.
		vk::SamplerAddressMode address_mode = vk::SamplerAddressMode::eRepeat;
	};

	// Represents a uniform binding for a material, including binding index, descriptor type, size, and data pointer.
	struct UniformBinding
	{
		/// Descriptor set binding index for the uniform.
		uint32_t binding;
		/// Vulkan descriptor type (e.g., uniform buffer, storage buffer).
		vk::DescriptorType type;
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
		vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
		/// Polygon mode (e.g., fill, line, point).
		vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
		/// Face culling mode.
		vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eNone;
		/// Winding order for front-facing polygons.
		vk::FrontFace front_face = vk::FrontFace::eClockwise;
		
		// Blending

		/// Enables or disables color blending.
		bool blend_enable = true;
		/// Source blend factor for color blending.
		vk::BlendFactor src_color_blend = vk::BlendFactor::eOne;
		/// Destination blend factor for color blending.
		vk::BlendFactor dst_color_blend = vk::BlendFactor::eDstAlpha;
		/// Blend operation for color blending.
		vk::BlendOp color_blend_op = vk::BlendOp::eAdd;
		
		// Depth testing

		/// Enables or disables depth testing.
		bool depth_test = true;
		/// Enables or disables writing to the depth buffer.
		bool depth_write = true;
		/// Depth comparison operation.
		vk::CompareOp depth_compare = vk::CompareOp::eGreaterOrEqual;
	};
}

#endif // _EAGE_GRAPHICS_MATERIAL_PROPERTY_H_