#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <chrono>

#include "GameMaterials.h"

namespace eage::graphics
{
	class RenderComponent;;
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

namespace audio
{
	class AudioMixer;
}

namespace eage::ecs
{
	class ECSRegistry;
	class RenderSystem;
}

namespace vortex
{
	class Player;

	class MainScene : public AbstractScene
	{
	public:
		MainScene( eage::graphics::Renderer& renderer, events::InputController& input_controller, 
				   audio::AudioMixer& audio_mixer, eage::ecs::ECSRegistry& ecs_registry,
				   eage::ecs::RenderSystem& render_system );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual void OnExit() override;

		virtual void Update() override;

		void PrepareMeshes();
		void PrepareMaterials();

	private:
		void DrawDebugGUI();

		eage::graphics::Renderer& mRenderer;
		events::InputController& mInputController;
		audio::AudioMixer& mAudioMixer;
		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::RenderSystem& mRenderSystem;

		vk::UniqueDescriptorSetLayout mSceneGlobalDataLayout;
		std::shared_ptr<eage::graphics::UniformDescriptor> mSceneGlobalDescriptor;
		std::unique_ptr<eage::graphics::ManagedBuffer, std::function<void(eage::graphics::ManagedBuffer*)>> mSceneGlobalDataDynamic;

		std::shared_ptr<eage::graphics::OrthographicCamera> mCamera;

		std::unique_ptr<SingleTextureSpriteMaterial> mSpriteMaterial;
		std::unique_ptr<SingleTextureSpriteMaterial::Resources> mSpriteMaterialResources;

		std::unique_ptr<Player> mPlayer;

		std::chrono::time_point<std::chrono::steady_clock> mLastUpdateTime;
	};
}

#endif // _MAIN_SCENE_H