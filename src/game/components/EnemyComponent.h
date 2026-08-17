#ifndef _VORTEX_ENEMY_COMPONENT_H_
#define _VORTEX_ENEMY_COMPONENT_H_

#include <string>

#include "../data/EnemyDefinition.h"

namespace vortex
{
	struct EnemyComponent
	{
		std::string definition_id;
		EnemyBehavior behavior = EnemyBehavior::DRIFT;
		float contact_damage = 50.f;
	};
}

#endif // _VORTEX_ENEMY_COMPONENT_H_
