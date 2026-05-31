#ifndef _GAMECONFIG_H
#define _GAMECONFIG_H

namespace vortex
{
	namespace config
	{
		enum class SceneID : int
		{
			MENU_SCENE = 0,
			MAIN_SCENE = 1
		};

		enum class DesignResolution
		{
			WIDTH = 1280,
			HEIGHT = 720
		};

		// Low-res render target for pixel-art style. Each "pixel" here maps to
		// (ScreenResolution / VirtualResolution) screen pixels, upscaled with nearest-neighbor.
		enum class VirtualResolution
		{
			WIDTH  = 640,
			HEIGHT = 360
		};

		struct ScreenResolution
		{
			int width = 1920;
			int height = 1080;
		};

		inline float get_scale_factor( int screen_width, int design_width )
		{
			return static_cast<float>( screen_width ) / static_cast<float>( design_width );
		}

		enum class GameEvents : uint64_t
		{
			PLAYER_ROTATE_LEFT = 0,
			PLAYER_ROTATE_RIGHT,
			PLAYER_THRUST,
			PLAYER_SHOOT,
		};

		enum PhysicsCategoryBits : uint16_t
		{
			PHYSX_CAT_DEFAULT     = 0x0001,
			PHYSX_CAT_PLAYER      = 0x0002,
			PHYSX_CAT_SCREEN_ZONE = 0x0004,
			PHYSX_CAT_WARPABLE    = 0x0008,
			PHYSX_CAT_ENEMY       = 0x0010,
			PHYSX_CAT_BULLET      = 0x0020
		};
	}
}

#endif // _GAMECONFIG_H