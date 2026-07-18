#ifndef _VORTEX_BULLET_COMPONENT_H_
#define _VORTEX_BULLET_COMPONENT_H_

namespace vortex
{
	enum class BulletState
	{
		Inactive,
		Alive,
		Dying
	};

	struct BulletComponent
	{
		BulletState state = BulletState::Inactive;
		float damage = 10.f;
		float lifetime_sec = 0.f;
		float age_sec = 0.f;
	};
}

#endif // _VORTEX_BULLET_COMPONENT_H_
