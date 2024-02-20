#ifndef _VORTEX_GAME_H
#define _VORTEX_GAME_H

#include <memory>

struct SDL_Window;

namespace vortex
{
	class VortexGame
	{
	public:
		VortexGame();
		~VortexGame();

		bool Init();

		void Run();

	private:
		std::shared_ptr<SDL_Window> mWindow;
	};
}

#endif // _VORTEX_GAME_H