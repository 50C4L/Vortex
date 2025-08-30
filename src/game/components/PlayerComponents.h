#ifndef VORTEX_PLAYER_COMPONENTS_H_
#define VORTEX_PLAYER_COMPONENTS_H_

#include <glm/glm.hpp>

namespace vortex
{
	struct PlayerComponent
	{
		// Movement state
		bool turning_left = false;
		bool turning_right = false;
		bool thrusting = false;

		// Player properties
		glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
		float max_thrust_speed = 200.f;
		float thrust_acceleration = 100.f;
		float rotation_speed = 200.f; // degrees per second
	};
}

#endif // VORTEX_PLAYER_COMPONENTS_H_