#ifndef _VULKAN_MESH_H
#define _VULKAN_MESH_H

#include <glm/glm.hpp>

#include <graphics/ManagedVulkanResources.h>

namespace graphics
{
	struct Vertex
	{
		glm::vec3 position;
		float uv_x;
		glm::vec3 normal;
		float uv_y;
		glm::vec4 color;
	};

	struct GPUMeshBuffers
	{
		ManagedBuffer::Ptr index_buffer;
		ManagedBuffer::Ptr vertex_buffer;
		vk::DeviceAddress vertex_buffer_address;
	};

	struct GPUImageBuffers
	{
		ManagedImage::Ptr image_buffer;
	};
}

#endif // _VULKAN_MESH_H