#ifndef _EAGE_RENDER_SYSTEM_H_
#define _EAGE_RENDER_SYSTEM_H_

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <ecs/ECS.h>
#include <ecs/ResourceManager.h>

namespace eage::graphics
{
	class Renderer;
	class AbstractCamera;
	struct MaterialProperty;
	struct Vertex;
}

namespace eage::ecs
{
	///
	/// RenderSystem: Manages rendering resources and convert ECS components to RenderInfo for rendering.
	/// Uses PIMPL to isolate Vulkan types from the public header.
	///
	class RenderSystem
	{
	public:
		RenderSystem( eage::graphics::Renderer& renderer, eage::ecs::ECSRegistry& ecs_registry );
		~RenderSystem();

		RenderSystem( const RenderSystem& ) = delete;
		RenderSystem& operator=( const RenderSystem& ) = delete;

		// Resource creation
		ResourceId CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices,
									 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset );
		ResourceId CreateMaterial( const eage::graphics::MaterialProperty& property );
		ResourceId CreateImageBuffer( const std::string& file_path );
		ResourceId CreateSpriteMesh( float width, float height, glm::vec2 uv_min, glm::vec2 uv_max );

		// Entity helpers
		void AttachRenderable( eage::ecs::Entity entity, ResourceId mesh_id, ResourceId material_id, bool visible = true );
		void AttachSprite( eage::ecs::Entity entity, ResourceId material_id, float width, float height, glm::vec2 uv_min, glm::vec2 uv_max, bool visible = true );

		// Camera
		void SetCamera( const eage::graphics::AbstractCamera& camera );

		void Update();

	private:
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	};
}

#endif // _EAGE_RENDER_SYSTEM_H_