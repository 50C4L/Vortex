#ifndef _RENDERABLE_H
#define _RENDERABLE_H

#include <memory>
#include <glm/glm.hpp>

namespace graphics
{
	struct GPUMeshBuffers;

	///
	/// Renderable class
	///
	class Renderable
	{
	public:
		struct UniformData
		{
			glm::mat4 model_matrix;
			uint64_t vertex_buffer_address;
		};
		///
		/// Constructor
		///
		Renderable();

		///
		/// Destructor
		///
		virtual ~Renderable();

		void SetMeshBuffer( std::shared_ptr<GPUMeshBuffers> mesh_buffer );
		const GPUMeshBuffers* GetMeshBuffer() const;

		const UniformData& GetFixedUniformData() const;

		void Rotate( float angle, const glm::vec3& axis );

	private:
		std::shared_ptr<GPUMeshBuffers> mMeshBuffer;
		UniformData mFixedUniformData;

	};
} // namespace graphics

#endif // _RENDERABLE_H