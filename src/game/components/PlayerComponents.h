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
		bool thruster_on = false;

		// Attack state
		bool main_weapon_firing = false;

		// Player properties
		glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
		float thrust_acceleration = 500.f;
		float rotation_speed = 3.f; // degrees per second
		int kill_count = 0;

		uint64_t thruster_fx_entity = 0; // Entity ID of the thruster effect
		uint64_t bullet_launcher_entity = 0; // Entity ID of the bullet spawn point (ship tip)
	};
}

#endif // VORTEX_PLAYER_COMPONENTS_H_