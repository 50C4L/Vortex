#include "Renderable.h"

#include <iostream>

#include <graphics/VulkanMesh.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace graphics;

Renderable::Renderable()
{
}

Renderable::~Renderable()
{
	std::cout << "Renderable::~Renderable" << std::endl;
}

void
Renderable::SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer )
{
	mMeshBuffer = std::move( mesh_buffer );
	mFixedUniformData.vertex_buffer_address = mMeshBuffer->vertex_buffer_address;
}

const GPUMeshBuffers*
Renderable::GetMeshBuffer() const
{
	return mMeshBuffer.get();
}

const Renderable::UniformData&
Renderable::GetFixedUniformData() const
{
	return mFixedUniformData;
}

void
Renderable::Rotate( float angle, const glm::vec3& axis )
{
	mFixedUniformData.model_matrix = glm::rotate( mFixedUniformData.model_matrix, angle, axis );
}