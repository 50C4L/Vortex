#ifndef _RenderComponent_H
#define _RenderComponent_H

#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <graphics/RenderInfo.h>

namespace graphics
{
	class Renderer;
	struct GPUMeshBuffers;
	struct ManagedBuffer;
	struct Material;
	class UniformDescriptor;

	///
	/// RenderComponent class
	///
	class RenderComponent
	{
	public:
		///
		/// Constructor
		///
		RenderComponent( Renderer& renderer );

		///
		/// Destructor
		///
		virtual ~RenderComponent();

		void SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer,
							uint32_t first_index,
							uint32_t index_count,
							uint32_t vertex_offset );

		const glm::mat4& GetTransformMatrix() const;

		glm::mat4 Rotate( float angle, const glm::vec3& axis, bool local );
		void Translate( const glm::vec3& translation );
		void Transform( const glm::mat4& transform );

		void SetMaterial( std::shared_ptr<Material> material );

		RenderInfo CreateRenderInfo();

	private:
		Renderer& mRenderer;
		std::shared_ptr<Material> mMaterial;
		glm::mat4 mTranslateMatrix;
		glm::mat4 mRotationMatrix;
		glm::mat4 mTransformMatrix;
		std::shared_ptr<GPUMeshBuffers> mMeshBuffer;

		std::unique_ptr<graphics::ManagedBuffer, std::function<void(graphics::ManagedBuffer*)>> mMeshUniformDataDynamic;
		std::unique_ptr<UniformDescriptor> mMeshDescriptor;
		
		uint32_t mFirstIndex   = 0;
		uint32_t mIndexCount   = 0;
		uint32_t mVertexOffset = 0;
	};
} // namespace graphics

#endif // _RenderComponent_H