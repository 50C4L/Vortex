#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"
#include <ecs/ResourceManager.h>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <chrono>

namespace eage::graphics
{
	class Renderer;
	class OrthographicCamera;
	class UniformDescriptor;
	struct ManagedBuffer;
	struct GPUImageBuffers;
	struct GPUMeshBuffers;
	struct RenderPipeline;
	class VulkanSampler;
}

namespace events
{
	class InputController;
}

namespace eage::ecs
{
	class AudioSystem;
	class ECSRegistry;
	class RenderSystem;
}

namespace vortex
{
	class Player;
	class PlayerInputSystem;
	class PlayerMovementSystem;

	class MainScene : public AbstractScene
	{
	public:
		MainScene( eage::graphics::Renderer& renderer, events::InputController& input_controller, 
				   eage::ecs::ECSRegistry& ecs_registry,
				   eage::ecs::AudioSystem& audio_system, eage::ecs::RenderSystem& render_system );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual uint64_t GetSceneRoot() override;
		virtual void OnExit() override;
		virtual void Update() override;

		void PrepareMeshes();
		void PrepareMaterials();

	private:
		void DrawDebugGUI();

		void CreateSceneRoot();
		void CreatePlayerEntity();

		eage::graphics::Renderer& mRenderer;
		events::InputController& mInputController;
		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::AudioSystem& mAudioSystem;
		eage::ecs::RenderSystem& mRenderSystem;

		std::unique_ptr<PlayerInputSystem> mPlayerInputSystem;
		std::unique_ptr<PlayerMovementSystem> mPlayerMovementSystem;

		std::shared_ptr<eage::graphics::OrthographicCamera> mCamera;

		eage::ecs::ResourceId mSpriteMaterialId;

		uint64_t mSceneRootEntity = 0;
		uint64_t mPlayerEntity = 0;

		std::chrono::time_point<std::chrono::steady_clock> mLastUpdateTime;
	};
}

#endif // _MAIN_SCENE_H