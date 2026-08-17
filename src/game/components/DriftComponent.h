#ifndef _VORTEX_DRIFT_COMPONENT_H_
#define _VORTEX_DRIFT_COMPONENT_H_

namespace vortex
{
	struct DriftComponent
	{
		float speed_min = 50.f;
		float speed_max = 150.f;
		float angular_speed_max = 10.f;
	};
}

#endif // _VORTEX_DRIFT_COMPONENT_H_
