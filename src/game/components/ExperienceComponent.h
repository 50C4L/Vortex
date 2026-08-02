#ifndef VORTEX_EXPERIENCE_COMPONENT_H_
#define VORTEX_EXPERIENCE_COMPONENT_H_

namespace vortex
{
	struct ExperienceComponent
	{
		int current_xp = 0;
		int xp_to_lvl_up = 5;
		int current_level = 1;
		int pending_xp = 0; // Queued by kill sites; drained by LevelingSystem
	};
}

#endif // VORTEX_EXPERIENCE_COMPONENT_H_
