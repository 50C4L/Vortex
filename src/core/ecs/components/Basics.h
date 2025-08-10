#ifndef _EAGE_COMPONENTS_BASICS_H_
#define _EAGE_COMPONENTS_BASICS_H_

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eage
{
namespace ecs
{
	struct TransformComponent
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat();
		glm::vec3 scale = glm::vec3(1.0f);

		glm::mat4 ToMatrix() const 
		{
			return glm::translate( glm::mat4(1.0f), position ) *
				   glm::toMat4( rotation ) *
				   glm::scale( glm::mat4(1.0f), scale );
		}
	};

	struct VelocityComponent
	{
 		glm::vec3 velocity = glm::vec3(0.0f);
	};
}
}

#endif // _EAGE_COMPONENTS_BASICS_H_