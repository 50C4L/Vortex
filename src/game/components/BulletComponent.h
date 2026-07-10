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
	};
}

#endif // _VORTEX_BULLET_COMPONENT_H_
