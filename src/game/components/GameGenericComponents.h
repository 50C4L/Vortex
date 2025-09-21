#ifndef _VORTEX_GAME_GENERIC_COMPONENTS_H_
#define _VORTEX_GAME_GENERIC_COMPONENTS_H_

namespace vortex
{
	///
	/// Entity with this component will be warpped when outside of the screen edges
	///
	struct WarpComponent
	{
	};

	struct WarpBoundaryComponent
	{
		// Pixels
		float left = -400.0f;
		float right = 400.0f;
		float top = 300.0f;
		float bottom = -300.0f;
		float wrap_offset = 2.0f;  // How far inside the opposite edge to place warped objects
	};
}

#endif // _VORTEX_GAME_GENERIC_COMPONENTS_H_