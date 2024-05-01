#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"

#include <vulkan/vulkan.hpp>

#include <memory>

#include "GameMaterials.h"

namespace graphics
{
	class RenderComponent;;
	class Renderer;
	class OrthographicCamera;
	class UniformDescriptor;
	struct ManagedBuffer;
	struct GPUImageBuffers;
	struct RenderPipeline;
	class VulkanSampler;
}

namespace vortex
{
	class MainScene : public AbstractScene
	{
	public:
		MainScene( graphics::Renderer& renderer );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual void OnExit() override;

		virtual void Update() override;

	private:
		graphics::Renderer& mRenderer;
		vk::UniqueDescriptorSetLayout mSceneGlobalDataLayout;
		std::shared_ptr<graphics::UniformDescriptor> mSceneGlobalDescriptor;
		std::vector<std::unique_ptr<graphics::ManagedBuffer, std::function<void(graphics::ManagedBuffer*)>>> mSceneGlobalData;

		vk::UniqueDescriptorSetLayout mRenderComponentDataLayout;
		std::vector<std::unique_ptr<graphics::ManagedBuffer, std::function<void(graphics::ManagedBuffer*)>>> mRenderComponentlData;

		std::shared_ptr<graphics::RenderComponent> mPlayer;
		std::shared_ptr<graphics::OrthographicCamera> mCamera;

		std::unique_ptr<SingleTextureSpriteMaterial> mSpriteMaterial;
		std::unique_ptr<SingleTextureSpriteMaterial::Resources> mSpriteMaterialResources;
	};
}

#endif // _MAIN_SCENE_H