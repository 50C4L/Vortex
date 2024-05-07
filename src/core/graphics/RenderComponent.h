#ifndef _RenderComponent_H
#define _RenderComponent_H

#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace graphics
{
	struct GPUMeshBuffers;
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
		RenderComponent();

		///
		/// Destructor
		///
		virtual ~RenderComponent();

		void SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer, uint32_t first_index, uint32_t index_count, uint32_t vertex_offset );
		const GPUMeshBuffers* GetMeshBuffer() const;
		void SetMeshDescriptor( std::unique_ptr<UniformDescriptor> mesh_descriptor );
		UniformDescriptor& GetMeshDescriptor();

		const glm::mat4 GetModelMatrix() const;

		void Rotate( float angle, const glm::vec3& axis );

		void SetMaterial( std::shared_ptr<Material> material );
		Material& GetMaterial();

		void Draw( vk::CommandBuffer& cmd );

	private:
		std::shared_ptr<Material> mMaterial;
		glm::mat4 mTransformMatrix;
		std::shared_ptr<GPUMeshBuffers> mMeshBuffer;
		std::unique_ptr<UniformDescriptor> mMeshDescriptor;
		
		uint32_t mFirstIndex   = 0;
		uint32_t mIndexCount   = 0;
		uint32_t mVertexOffset = 0;
	};
} // namespace graphics

#endif // _RenderComponent_H