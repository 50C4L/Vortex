#ifndef _EAGE_RENDER_SYSTEM_H_
#define _EAGE_RENDER_SYSTEM_H_

#include <ecs/ECS.h>
#include <ecs/ResourceManager.h>
#include <graphics/RenderInfo.h>
#include <graphics/VulkanMesh.h>
#include <graphics/Material.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanDescriptor.h>

namespace eage::graphics
{
	class Renderer;
}

namespace eage::ecs
{
	class RenderSystem
	{
	public:
		RenderSystem( eage::graphics::Renderer& renderer, eage::ecs::ECSRegistry& ecs_registry );
		~RenderSystem();

		// Resource creation methods
		ResourceID CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices,
									 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset );
		ResourceID CreateMaterial(/* material parameters */);
		ResourceID CreateUniformBuffer(/* uniform data */);
		ResourceID CreateDescriptorSet(/* descriptor parameters */);

		void Update();
		void Render();

		// Entity lifecycle
		void OnEntityDestroyed(eage::ecs::Entity entity);

	private:
		eage::graphics::RenderInfo CreateRenderInfo( eage::ecs::Entity entity );
		
		eage::graphics::Renderer& mRenderer;
		eage::ecs::ECSRegistry& mECSRegistry;

		// Resource managers
		ResourceManager<eage::graphics::GPUMeshBuffers> mMeshBuffers;
		ResourceManager<eage::graphics::Material> mMaterials;
		ResourceManager<eage::graphics::ManagedBuffer> mUniformBuffers;
		ResourceManager<eage::graphics::UniformDescriptor> mDescriptorSets;
	};
}

#endif // _EAGE_RENDER_SYSTEM_H_