#ifndef _VORTEX_GAME_H
#define _VORTEX_GAME_H

#include <memory>

struct SDL_Window;

namespace eage::graphics
{
	class Renderer;
	class SceneRenderPass;
	class ImGuiRenderPass;
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
	class PhysicsSystem;
	class RenderSystem;
	class SceneGraphSystem;
}

namespace eage::profiling
{
	class PerformanceTracker;
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
		std::unique_ptr<eage::graphics::SceneRenderPass> mScenePass;
		std::unique_ptr<eage::graphics::ImGuiRenderPass> mImGuiPass;
		std::unique_ptr<SceneController> mSceneController;
		std::unique_ptr<events::InputController> mInputController;
		std::unique_ptr<eage::audio::AudioMixer> mAudioMixer;
		std::unique_ptr<eage::ecs::ECSRegistry> mECSRegistry;
		std::unique_ptr<eage::ecs::AudioSystem> mAudioSystem;
		std::unique_ptr<eage::ecs::SceneGraphSystem> mSceneGraphSystem;
		std::unique_ptr<eage::ecs::RenderSystem> mRenderSystem;
		std::unique_ptr<eage::ecs::PhysicsSystem> mPhysicsSystem;
		std::unique_ptr<eage::profiling::PerformanceTracker> mPerformanceTracker;
	};
}

#endif // _VORTEX_GAME_H