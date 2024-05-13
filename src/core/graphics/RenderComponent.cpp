#include "RenderComponent.h"

#include <iostream>

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

const GPUMeshBuffers*
RenderComponent::GetMeshBuffer() const
{
	return mMeshBuffer.get();
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
	mTransformMatrix = glm::rotate( mTransformMatrix, glm::radians( angle ), axis );
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

void
RenderComponent::Update()
{
	if( !mMeshBuffer )
	{
		return;
	}

	auto current_frame = mRenderer.GetCurrentFrameIndex();

	MeshUniformData data;
	data.model = mTransformMatrix;
	data.vertex_buffer_address = mMeshBuffer->vertex_buffer_address;
	mMeshUniformDataDynamic->Update( &data, sizeof( MeshUniformData ), sizeof( MeshUniformData ) * current_frame );
}