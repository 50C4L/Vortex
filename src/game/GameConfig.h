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
			WIDTH = 1920,
			HEIGHT = 1080
		};

		enum class GameEvents : uint64_t
		{
			PLAYER_ROTATE_LEFT = 0,
			PLAYER_ROTATE_RIGHT,
			PLAYER_THRUST,
		};

		enum PhysicsCategoryBits : uint16_t
		{
			PHYSX_CAT_DEFAULT = 0x0001,
			PHYSX_CAT_PLAYER = 0x0002,
			PHYSX_CAT_SCREEN_ZONE = 0x0003,
			PHYSX_CAT_WARPABLE = 0x0004,
			PHYSX_CAT_ENEMY = 0x0005
		};
	}
}

#endif // _GAMECONFIG_H