#include "RenderComponent.h"

#include <iostream>

#include <graphics/BuiltInUniforms.h>
#include <graphics/Renderer.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VMAWrapper.h>
#include <graphics/ManagedVulkanResources.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace graphics;

RenderComponent::RenderComponent( Renderer& renderer )
	: mRenderer( renderer )
	, mModelMatrix( 1.0f )
	, mTranslateMatrix( 1.0f )
	, mRotationMatrix( 1.0f )
	, mTransformMatrix( 1.0f )
{
	const auto num_overlapping_frames = mRenderer.GetFrames().size();
	mMeshUniformDataDynamic = ManagedBuffer::Create( 
		*mRenderer.GetMemoryAllocator().allocator.get(), 
		sizeof( MeshUniformData ) * num_overlapping_frames, 
		vk::BufferUsageFlagBits::eUniformBuffer, 
		VMA_MEMORY_USAGE_CPU_TO_GPU 
	);

	mMeshDescriptor = std::make_unique<graphics::UniformDescriptor>( mRenderer, mRenderer.GetBuiltInDescriptorSetLayouts().render_component.get() );
	mMeshDescriptor->WriteDynamicBuffer( 0, vk::DescriptorType::eUniformBufferDynamic, mMeshUniformDataDynamic->buffer, sizeof( MeshUniformData ) );
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

const glm::mat4
RenderComponent::GetModelMatrix() const
{
	return mModelMatrix;
}

const glm::mat4&
RenderComponent::GetTranslateMatrix() const
{
	return mTranslateMatrix;
}

glm::mat4
RenderComponent::Rotate( float angle, const glm::vec3& axis, bool local )
{
	// https://stackoverflow.com/questions/21923482/rotate-and-translate-object-in-local-and-global-orientation-using-glm
	mRotationMatrix = glm::rotate( mRotationMatrix, glm::radians( angle ), axis );
	if( local )
	{
		mModelMatrix = mModelMatrix * mRotationMatrix;
	}
	else
	{
		mModelMatrix = mRotationMatrix * mModelMatrix;
	}
	return mRotationMatrix;
}

void
RenderComponent::Translate( const glm::vec3& translation )
{
	mTranslateMatrix = glm::translate( mTranslateMatrix, translation );
	mModelMatrix = mTranslateMatrix * mModelMatrix;
}

void
RenderComponent::SetMaterial( std::shared_ptr<Material> material )
{
	mMaterial = std::move( material );
}

RenderInfo
RenderComponent::CreateRenderInfo()
{
	RenderInfo render_info;
	render_info.material                  = mMaterial.get();
	render_info.mesh_buffer               = mMeshBuffer.get();
	render_info.mesh_descriptor           = mMeshDescriptor.get();
	render_info.mesh_uniform_data_dynamic = mMeshUniformDataDynamic.get();
	render_info.first_index               = mFirstIndex;
	render_info.index_count               = mIndexCount;
	render_info.vertex_offset             = mVertexOffset;
	return render_info;
}