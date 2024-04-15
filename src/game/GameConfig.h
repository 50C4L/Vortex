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
	}
}

#endif // _GAMECONFIG_H