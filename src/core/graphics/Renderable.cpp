#include "Renderable.h"

#include <iostream>

#include <graphics/VulkanMesh.h>
#include <graphics/VulkanDescriptor.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace graphics;

Renderable::Renderable( RenderPipeline& render_pipeline )
	: mRenderPipeline( render_pipeline )
	, mTransformMatrix( 1.0f )
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
}

const GPUMeshBuffers*
Renderable::GetMeshBuffer() const
{
	return mMeshBuffer.get();
}

void
Renderable::SetMeshDescriptor( std::unique_ptr<UniformDescriptor> mesh_descriptor )
{
	mMeshDescriptor = std::move( mesh_descriptor );
}

const UniformDescriptor&
Renderable::GetMeshDescriptor() const
{
	return *mMeshDescriptor;
}

const glm::mat4
Renderable::GetModelMatrix() const
{
	return mTransformMatrix;
}

void
Renderable::Rotate( float angle, const glm::vec3& axis )
{
	mTransformMatrix = glm::rotate( mTransformMatrix, angle, axis );
}

RenderPipeline&
Renderable::GetRenderPipeline()
{
	return mRenderPipeline;
}

void
Renderable::SetDrawIndexInfo( DrawIndexInfo draw_index_info )
{
	mDrawIndexInfo = std::move( draw_index_info );
}

const Renderable::DrawIndexInfo&
Renderable::GetDrawIndexInfo() const
{
	return mDrawIndexInfo;
}