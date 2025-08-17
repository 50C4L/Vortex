#ifndef _EAGE_RENDER_SYSTEM_H_
#define _EAGE_RENDER_SYSTEM_H_

#include <ecs/ECS.h>
#include <ecs/ResourceManager.h>
#include <graphics/RenderInfo.h>
#include <graphics/VulkanMesh.h>
#include <graphics/Material.h>
#include <graphics/MaterialProperty.h>
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
		ResourceId CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices,
									 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset );
		ResourceId CreateMaterial( const eage::graphics::MaterialProperty& property );
		ResourceId CreateUniformBuffer( size_t data_size, bool dynamic = false );
		ResourceId CreateDescriptorSet( vk::DescriptorSetLayout layout );

		// Resource accessors
		eage::graphics::ManagedBuffer* GetUniformBuffer(ResourceId id);

		void Update();
		void Render();

		// Entity lifecycle
		void OnEntityDestroyed(eage::ecs::Entity entity);

	private:
		eage::graphics::RenderInfo CreateRenderInfo( eage::ecs::Entity entity );

		std::shared_ptr<eage::graphics::RenderPipeline> CreateOrGetPipeline(
			const eage::graphics::MaterialProperty& property,
			const std::vector<vk::DescriptorSetLayout>& global_layouts );

		size_t HashMaterialProperty( const eage::graphics::MaterialProperty& property );
		
		eage::graphics::Renderer& mRenderer;
		eage::ecs::ECSRegistry& mECSRegistry;

		// Resource managers
		ResourceManager<std::unique_ptr<eage::graphics::GPUMeshBuffers>> mMeshBuffers;
		ResourceManager<std::unique_ptr<eage::graphics::Material>> mMaterials;
		ResourceManager<eage::graphics::ManagedBuffer::Ptr> mUniformBuffers;
		ResourceManager<std::unique_ptr<eage::graphics::UniformDescriptor>> mDescriptorSets;

		std::unordered_map<size_t, std::shared_ptr<eage::graphics::RenderPipeline>> mPipelineCache;
	};
}

#endif // _EAGE_RENDER_SYSTEM_H_