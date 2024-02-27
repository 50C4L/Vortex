#ifndef _VORTEX_GAME_H
#define _VORTEX_GAME_H

#include <memory>

struct SDL_Window;

namespace graphics
{
	class Renderer;
}

namespace vortex
{
	class SceneController;

	class VortexGame
	{
	public:
		VortexGame();
		virtual ~VortexGame();

		bool Init();

		void Run();

	private:
		std::shared_ptr<SDL_Window> mWindow;
		std::unique_ptr<graphics::Renderer> mRenderer;
		std::unique_ptr<SceneController> mSceneController;
	};
}

#endif // _VORTEX_GAME_H