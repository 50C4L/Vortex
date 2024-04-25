#ifndef _RENDERABLE_H
#define _RENDERABLE_H

#include <memory>
#include <glm/glm.hpp>

namespace graphics
{
	struct GPUMeshBuffers;
	struct RenderPipeline;
	class UniformDescriptor;

	///
	/// Renderable class
	///
	class Renderable
	{
	public:
		///
		/// Constructor
		///
		Renderable( RenderPipeline& render_pipeline );

		///
		/// Destructor
		///
		virtual ~Renderable();

		void SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer );
		const GPUMeshBuffers* GetMeshBuffer() const;
		void SetMeshDescriptor( std::unique_ptr<UniformDescriptor> mesh_descriptor );
		const UniformDescriptor& GetMeshDescriptor() const;

		const glm::mat4 GetModelMatrix() const;

		void Rotate( float angle, const glm::vec3& axis );

		RenderPipeline& GetRenderPipeline();

		struct DrawIndexInfo
		{
			uint32_t first_index   = 0;
			uint32_t index_count   = 0;
			uint32_t vertex_offset = 0;
		};
		void SetDrawIndexInfo( DrawIndexInfo draw_index_info );
		const DrawIndexInfo& GetDrawIndexInfo() const;

	private:
		RenderPipeline& mRenderPipeline;
		glm::mat4 mTransformMatrix;
		std::shared_ptr<GPUMeshBuffers> mMeshBuffer;
		std::unique_ptr<UniformDescriptor> mMeshDescriptor;
		DrawIndexInfo mDrawIndexInfo;
	};
} // namespace graphics

#endif // _RENDERABLE_H