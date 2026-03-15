#ifndef _VORTEX_BULLET_COMPONENT_H_
#define _VORTEX_BULLET_COMPONENT_H_

namespace vortex
{
	struct BulletComponent
	{
		bool is_alive = false;
		float damage = 10.f;
	};
}

#endif // _VORTEX_BULLET_COMPONENT_H_
