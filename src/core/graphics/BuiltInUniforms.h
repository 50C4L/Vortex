#ifndef _EAGE_BUILT_IN_UNIFORMS_H_
#define _EAGE_BUILT_IN_UNIFORMS_H_

#include <glm/glm.hpp>

namespace eage::graphics
{
	struct MeshUniformData
	{
		alignas(64) glm::mat4 model;
		alignas(8) uint64_t vertex_buffer_address;
		// padding
		float extra[46];
	};
}

#endif // _EAGE_BUILT_IN_UNIFORMS_H_