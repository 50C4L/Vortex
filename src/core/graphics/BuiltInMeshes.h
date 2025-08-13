#ifndef _EAGE_BUILT_IN_MESHES_H_
#define _EAGE_BUILT_IN_MESHES_H_

#include <graphics/VulkanMesh.h>

namespace eage::graphics
{
	struct BuiltInRect
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};
	BuiltInRect made_rect_vertices( glm::vec3 center, float width, float height );
}

#endif // _EAGE_BUILT_IN_MESHES_H_