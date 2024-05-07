#include "RenderComponent.h"

#include <iostream>

#include <graphics/VulkanMesh.h>
#include <graphics/VulkanDescriptor.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace graphics;

RenderComponent::RenderComponent()
	: mTransformMatrix( 1.0f )
{
}

RenderComponent::~RenderComponent()
{
}

void
RenderComponent::SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer, uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
{
	mMeshBuffer = std::move( mesh_buffer );
	mFirstIndex = first_index;
	mIndexCount = index_count;
	mVertexOffset = vertex_offset;
}

const GPUMeshBuffers*
RenderComponent::GetMeshBuffer() const
{
	return mMeshBuffer.get();
}

void
RenderComponent::SetMeshDescriptor( std::unique_ptr<UniformDescriptor> mesh_descriptor )
{
	mMeshDescriptor = std::move( mesh_descriptor );
}

UniformDescriptor&
RenderComponent::GetMeshDescriptor()
{
	return *mMeshDescriptor;
}

const glm::mat4
RenderComponent::GetModelMatrix() const
{
	return mTransformMatrix;
}

void
RenderComponent::Rotate( float angle, const glm::vec3& axis )
{
	mTransformMatrix = glm::rotate( mTransformMatrix, angle, axis );
}

void
RenderComponent::SetMaterial( std::shared_ptr<Material> material )
{
	mMaterial = std::move( material );
}

Material&
RenderComponent::GetMaterial()
{
	return *mMaterial;
}

void
RenderComponent::Draw( vk::CommandBuffer& cmd )
{
	if( !mMeshBuffer )
	{
		return;
	}

	cmd.bindIndexBuffer( mMeshBuffer->index_buffer->buffer, 0, vk::IndexType::eUint32 );
	cmd.drawIndexed( mIndexCount, 1, mFirstIndex, mVertexOffset, 0 );
}