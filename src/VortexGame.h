#ifndef _VORTEX_GAME_H
#define _VORTEX_GAME_H

#include <memory>

struct SDL_Window;

namespace eage::graphics
{
	class Renderer;
}

namespace events
{
	class InputController;
}

namespace eage::audio
{
	class AudioMixer;
}

namespace eage::ecs
{
	class AudioSystem;
	class ECSRegistry;
	class RenderSystem;
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
		std::unique_ptr<eage::graphics::Renderer> mRenderer;
		std::unique_ptr<SceneController> mSceneController;
		std::unique_ptr<events::InputController> mInputController;
		std::unique_ptr<eage::audio::AudioMixer> mAudioMixer;
		std::unique_ptr<eage::ecs::ECSRegistry> mECSRegistry;
		std::unique_ptr<eage::ecs::AudioSystem> mAudioSystem;
		std::unique_ptr<eage::ecs::RenderSystem> mRenderSystem;
	};
}

#endif // _VORTEX_GAME_H