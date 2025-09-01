#ifndef _EAGE_RENDER_SYSTEM_H_
#define _EAGE_RENDER_SYSTEM_H_

#include <assets/ImageLoader.h>
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

	struct SceneGlobalData
	{
		alignas(64) glm::mat4 view;
		alignas(64) glm::mat4 proj;
		alignas(64) glm::mat4 view_proj;
		// padding
		float extra[16];
	};
}

namespace eage::ecs
{
	///
	/// RenderSystem: Manages rendering resources and convert ECS components to RenderInfo for rendering
	///
	class RenderSystem
	{
	public:
		RenderSystem( eage::graphics::Renderer& renderer, eage::ecs::ECSRegistry& ecs_registry );
		~RenderSystem();

		// Resource creation methods
		ResourceId CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices,
									 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset );
		ResourceId CreateMaterial( const eage::graphics::MaterialProperty& property );
		ResourceId CreateUniformBuffer( size_t data_size );
		ResourceId CreateImageBuffer( const std::string& file_path );
		ResourceId CreateDynamicUniformBuffer( size_t data_size );
		ResourceId CreateDescriptorSet( vk::DescriptorSetLayout layout );
		ResourceId CreateDynamicDescriptorSet( vk::DescriptorSetLayout layout );
		ResourceId CreateSampler( vk::Filter min_filter, vk::Filter mag_filter );

		// Resource accessors
		eage::graphics::ManagedBuffer* GetGlobalUniformBuffer();
		eage::graphics::ManagedImage* GetImageBuffer( ResourceId id );
		eage::graphics::ManagedBuffer* GetUniformBuffer( ResourceId id );
		eage::graphics::AbstractUniformDescriptor* GetDescriptorSet( ResourceId id );
		vk::Sampler GetSampler( ResourceId id );

		void Update();

	private:
		std::shared_ptr<eage::graphics::RenderPipeline> CreateOrGetPipeline(
			const eage::graphics::MaterialProperty& property,
			const std::vector<vk::DescriptorSetLayout>& global_layouts );

		size_t HashMaterialProperty( const eage::graphics::MaterialProperty& property );
		
		eage::graphics::Renderer& mRenderer;
		eage::ecs::ECSRegistry& mECSRegistry;

		ResourceId mGlobalDescriptorSetId;
		ResourceId mGlobalUniformBufferId;

		// Resource managers
		ResourceManager<std::unique_ptr<eage::graphics::GPUMeshBuffers>> mMeshBuffers;
		ResourceManager<std::unique_ptr<eage::graphics::Material>> mMaterials;
		ResourceManager<eage::graphics::ManagedBuffer::Ptr> mUniformBuffers;
		ResourceManager<eage::graphics::ManagedImage::Ptr> mImages;
		ResourceManager<std::unique_ptr<eage::graphics::AbstractUniformDescriptor>> mDescriptorSets;
		ResourceManager<std::unique_ptr<vk::UniqueSampler>> mSamplers;

		std::unordered_map<size_t, std::shared_ptr<eage::graphics::RenderPipeline>> mPipelineCache;
		std::unordered_map<std::string, ResourceId> mImagePathToIdMap;
		std::unordered_map<size_t, ResourceId> mSamplerCache;
	};
}

#endif // _EAGE_RENDER_SYSTEM_H_