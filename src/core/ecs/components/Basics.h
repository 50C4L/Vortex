#ifndef _EAGE_COMPONENTS_BASICS_H_
#define _EAGE_COMPONENTS_BASICS_H_

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eage::ecs
{
	struct TransformComponent
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat();
		glm::vec3 scale = glm::vec3(1.0f);

		// Cached matrix for efficiency
		glm::mat4 local_matrix = glm::mat4(1.0f);
		glm::mat4 world_matrix = glm::mat4(1.0f); // World matrix is updated by SceneGraphSystem
		bool dirty = true;

		void MarkDirty()
		{
			dirty = true;
		}

		void SetPosition( const glm::vec3& pos )
		{
			position = pos;
			MarkDirty();
		}

		void SetRotation( const glm::quat& rot )
		{
			rotation = rot;
			MarkDirty();
		}

		void SetScale( const glm::vec3& scl )
		{
			scale = scl;
			MarkDirty();
		}

		glm::mat4 GetLocalMatrix()
		{
			if( dirty )
			{
				local_matrix = glm::translate( glm::mat4(1.0f), position ) *
							   glm::toMat4( rotation ) *
							   glm::scale( glm::mat4(1.0f), scale );
				dirty = false;
			}
			return local_matrix;
		}

		glm::mat4 GetWorldMatrix()
		{
			return world_matrix;
		}

		void SetWorldMatrix( const glm::mat4& mat )
		{
			world_matrix = mat;
		}
	};

	struct Velocity2DComponent
	{
 		glm::vec3 velocity = glm::vec3(0.0f);
		float angular_velocity = 0.0f; // In degrees per second
	};

	struct RelationshipComponent
	{
		uint64_t parent_entity = 0; // 0 means no parent
		std::vector<uint64_t> children_entities;
	};
}

#endif // _EAGE_COMPONENTS_BASICS_H_