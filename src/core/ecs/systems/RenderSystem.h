#ifndef _EAGE_RENDER_SYSTEM_H_
#define _EAGE_RENDER_SYSTEM_H_

#include <ecs/ECS.h>
#include <ecs/ResourceManager.h>
#include <graphics/RenderInfo.h>
#include <graphics/VulkanMesh.h>
#include <graphics/Material.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanDescriptor.h>

namespace graphics
{
	class Renderer;
}

namespace eage::ecs
{
	class RenderSystem
	{
	public:
		RenderSystem( graphics::Renderer& renderer, eage::ecs::ECSRegistry& ecs_registry );
		~RenderSystem();

		// Resource creation methods
		ResourceID CreateMeshBuffer(/* mesh data parameters */);
		ResourceID CreateMaterial(/* material parameters */);
		ResourceID CreateUniformBuffer(/* uniform data */);
		ResourceID CreateDescriptorSet(/* descriptor parameters */);

		void Update();
		void Render();

		// Entity lifecycle
		void OnEntityDestroyed(eage::ecs::Entity entity);

	private:
		graphics::RenderInfo CreateRenderInfo( eage::ecs::Entity entity );
		
		graphics::Renderer& mRenderer;
		eage::ecs::ECSRegistry& mECSRegistry;

		// Resource managers
		ResourceManager<graphics::GPUMeshBuffers> mMeshBuffers;
		ResourceManager<graphics::Material> mMaterials;
		ResourceManager<graphics::ManagedBuffer> mUniformBuffers;
		ResourceManager<graphics::UniformDescriptor> mDescriptorSets;
	};
}

#endif // _EAGE_RENDER_SYSTEM_H_