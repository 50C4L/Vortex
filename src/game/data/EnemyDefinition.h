#ifndef _VORTEX_ENEMY_DEFINITION_H_
#define _VORTEX_ENEMY_DEFINITION_H_

#include <string>

namespace vortex
{
	enum class EnemyBehavior
	{
		DRIFT
	};

	struct DriftParams
	{
		float speed_min = 50.f;
		float speed_max = 150.f;
		float angular_speed_max = 10.f;
	};

	struct EnemyDefinition
	{
		int version = 1;
		std::string id;
		EnemyBehavior behavior = EnemyBehavior::DRIFT;
		std::string texture_path;
		float sprite_width = 32.f;
		float sprite_height = 32.f;
		float collider_radius = 16.f;
		float max_health = 1.f;
		float contact_damage = 50.f;
		float max_linear_velocity = 150.f;
		int xp_reward = 0;
		bool warpable = true;
		DriftParams drift;
	};

	bool parse_enemy_behavior( const std::string& name, EnemyBehavior& out );
	bool load_enemy_definition( const std::string& path, EnemyDefinition& out );
}

#endif // _VORTEX_ENEMY_DEFINITION_H_
