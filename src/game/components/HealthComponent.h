#ifndef VORTEX_HEALTH_COMPONENT_H_
#define VORTEX_HEALTH_COMPONENT_H_

namespace vortex
{
	struct HealthComponent
	{
		float max_health = 100.f;
		float health = 100.f;
		float pending_damage = 0.f; // Accumulated damage this frame, reset after processing

		bool IsDead() const { return health <= 0.f; }
	};
}

#endif // VORTEX_HEALTH_COMPONENT_H_
