#ifndef _EAGE_BUILT_IN_UNIFORMS_H_
#define _EAGE_BUILT_IN_UNIFORMS_H_

#include <glm/glm.hpp>

namespace eage::graphics
{
	struct MeshUniformData
	{
		alignas(64) glm::mat4 model;
		alignas(8) uint64_t vertex_buffer_address;
		alignas(16) glm::vec4 uv_rect; // xy = uv offset, zw = uv scale
		// padding
		float extra[38];
	};

	struct SceneGlobalData
	{
		alignas(64) glm::mat4 view;
		alignas(64) glm::mat4 proj;
		alignas(64) glm::mat4 view_proj;
		// padding
		float extra[16];
	};
}

#endif // _EAGE_BUILT_IN_UNIFORMS_H_