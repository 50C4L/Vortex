#ifndef _EAGE_RENDER_INFO_H_
#define _EAGE_RENDER_INFO_H_

#include <stdint.h>

namespace graphics
{
	struct GPUMeshBuffers;
	struct Material;
	class UniformDescriptor;
	struct MeshUniformData;

	struct RenderInfo
	{
		Material*          material;
		GPUMeshBuffers*    mesh_buffer;
		UniformDescriptor* mesh_descriptor;
		uint32_t           first_index;
		uint32_t           index_count;
		uint32_t           vertex_offset;
	};
}

#endif // _EAGE_RENDER_INFO_H_