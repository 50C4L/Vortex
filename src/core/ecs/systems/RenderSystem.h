#ifndef _EAGE_RENDER_SYSTEM_H_
#define _EAGE_RENDER_SYSTEM_H_

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <ecs/ECS.h>
#include <ecs/ResourceStore.h>

namespace eage::graphics
{
	class Renderer;
	class SceneRenderPass;
	class AbstractCamera;
	struct MaterialProperty;
	struct Vertex;
}

namespace eage::ecs
{
	using MeshHandle = ResourceHandle;
	using MaterialHandle = ResourceHandle;

	///
	/// RenderSystem: Manages rendering resources and convert ECS components to RenderInfo for rendering.
	/// Uses PIMPL to isolate Vulkan types from the public header.
	///
	class RenderSystem : public ECSRegistry::Observer
	{
	public:
		RenderSystem( eage::graphics::Renderer& renderer, eage::ecs::ECSRegistry& ecs_registry );
		~RenderSystem();

		RenderSystem( const RenderSystem& ) = delete;
		RenderSystem& operator=( const RenderSystem& ) = delete;

		// Resource creation -- returned handles Adopt the Store's initial ref.
		MeshHandle CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices,
									 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset );
		MaterialHandle CreateMaterial( const eage::graphics::MaterialProperty& property );
		uint32_t CreateTexture( const std::string& file_path );
		MeshHandle CreateSpriteMesh( float width, float height );

		// Entity helpers -- AttachRenderable AddReferences; components hold non-owning ResourceIds.
		void AttachRenderable( eage::ecs::Entity entity, ResourceId mesh_id, ResourceId material_id, uint32_t texture_index = 0, bool visible = true );
		void AttachSprite( eage::ecs::Entity entity, ResourceId material_id, float width, float height, uint32_t texture_index, bool visible = true );
		void DetachRenderable( eage::ecs::Entity entity );

		/// Free all deferred GPU resources immediately. Call only after WaitForIdle / when GPU is idle.
		void FlushPendingDeletes();

		// Camera
		void SetCamera( const eage::graphics::AbstractCamera& camera, glm::vec2 virtual_resolution );

		/// Retarget the scene pass that receives draw submissions. Pass nullptr to clear.
		void SetScenePass( eage::graphics::SceneRenderPass* scene_pass );

		void Update();

		// ECSRegistry::Observer
		void OnEntityDestroying( Entity entity ) override;

	private:
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	};
}

#endif // _EAGE_RENDER_SYSTEM_H_
