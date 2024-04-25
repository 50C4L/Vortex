#include "RenderComponent.h"

#include <iostream>

#include <graphics/VulkanMesh.h>
#include <graphics/VulkanDescriptor.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace graphics;

RenderComponent::RenderComponent( RenderPipeline& render_pipeline )
	: mRenderPipeline( render_pipeline )
	, mTransformMatrix( 1.0f )
{
}

RenderComponent::~RenderComponent()
{
}

void
RenderComponent::SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer )
{
	mMeshBuffer = std::move( mesh_buffer );
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

const UniformDescriptor&
RenderComponent::GetMeshDescriptor() const
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

RenderPipeline&
RenderComponent::GetRenderPipeline()
{
	return mRenderPipeline;
}

void
RenderComponent::SetDrawIndexInfo( DrawIndexInfo draw_index_info )
{
	mDrawIndexInfo = std::move( draw_index_info );
}

const RenderComponent::DrawIndexInfo&
RenderComponent::GetDrawIndexInfo() const
{
	return mDrawIndexInfo;
}