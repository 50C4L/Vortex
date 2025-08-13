#include "RenderSystem.h"

#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
#include <graphics/BuiltInUniforms.h>
#include <graphics/Renderer.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VMAWrapper.h>
#include <graphics/ManagedVulkanResources.h>

using namespace eage::ecs;

RenderSystem::RenderSystem( eage::graphics::Renderer& renderer, ECSRegistry& ecs_registry )
	: mRenderer(renderer)
	, mECSRegistry(ecs_registry)
{
}

RenderSystem::~RenderSystem()
{
}

ResourceID
RenderSystem::CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices, 
								uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
{
	auto mesh = mRenderer.UploadMesh( indices, vertices );
	mesh->first_index = first_index;
	mesh->index_count = index_count;
	mesh->vertex_offset = vertex_offset;

	return mMeshBuffers.Store( std::move(mesh) );
}

ResourceID
RenderSystem::CreateMaterial(/* material parameters */)
{
	return INVALID_ID;
}

ResourceID
RenderSystem::CreateUniformBuffer(/* uniform data */)
{
	return INVALID_ID;
}

ResourceID
RenderSystem::CreateDescriptorSet(/* descriptor parameters */)
{
	return INVALID_ID;
}


void RenderSystem::Update()
{
	// Update uniform buffers with transform data
	// This replaces the old RenderComponent::Transform logic
}

void RenderSystem::Render()
{
	// Iterate through all entities with both Transform and Render components
	// This replaces the old RenderComponent::CreateRenderInfo logic
	
	// For now, you'd need to iterate manually or implement a query system
	// Example pseudo-code:
	/*
	for (auto entity : entities_with_render_and_transform_components)
	{
		auto& transform = mECSRegistry.GetComponent<TransformComponent>(entity);
		auto& render = mECSRegistry.GetComponent<RenderComponent>(entity);
		
		if (render.visible)
		{
			// Update uniforms with transform.ToMatrix()
			// Create RenderInfo and add to render queue
			mRenderer.AddToRenderQueue(CreateRenderInfo(entity));
		}
	}
	*/
}

eage::graphics::RenderInfo RenderSystem::CreateRenderInfo(Entity entity)
{
	auto& render = mECSRegistry.GetComponent<RenderComponent>(entity);
	auto& transform = mECSRegistry.GetComponent<TransformComponent>(entity);
	
	// Update uniform buffer with transform matrix
	// Return RenderInfo similar to old implementation
	
	eage::graphics::RenderInfo info{};
	// Fill info with data from render component
	return info;
}