#ifndef _VORTEX_SHIP_COMPONENTS_H_
#define _VORTEX_SHIP_COMPONENTS_H_

namespace vortex
{
namespace components
{
	struct ShipStateComponent
	{
		bool is_thrust_on = false;
		// Add other ship-specific state flags as needed
	};
}
}

#endif // _VORTEX_SHIP_COMPONENTS_H_