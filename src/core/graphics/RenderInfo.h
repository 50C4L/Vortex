#ifndef _EAGE_RENDER_INFO_H_
#define _EAGE_RENDER_INFO_H_

#include <stdint.h>
#include <glm/glm.hpp>

namespace eage::graphics
{
	struct GPUMeshBuffers;
	struct Material;
	class AbstractUniformDescriptor;
	struct MeshUniformData;
	struct ManagedBuffer;

	struct RenderInfo
	{
		Material*          material;
		GPUMeshBuffers*    mesh_buffer;
		AbstractUniformDescriptor* mesh_descriptor;
		ManagedBuffer*     mesh_uniform_data_dynamic;
		uint32_t           first_index;
		uint32_t           index_count;
		uint32_t           vertex_offset;
		glm::mat4          model_matrix;
		glm::vec4          uv_rect = { 0.f, 0.f, 1.f, 1.f }; // xy = uv offset, zw = uv scale
	};
}

#endif // _EAGE_RENDER_INFO_H_