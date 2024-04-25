#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace graphics
{
	class Renderable;;
	class Renderer;
	class OrthographicCamera;
	class UniformDescriptor;
	struct ManagedBuffer;
	struct RenderPipeline;
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

		std::unique_ptr<graphics::RenderPipeline> mGeneralPipeline;
		vk::UniqueDescriptorSetLayout mRenderableDataLayout;
		std::vector<std::unique_ptr<graphics::ManagedBuffer, std::function<void(graphics::ManagedBuffer*)>>> mRenderablelData;

		std::shared_ptr<graphics::Renderable> mPlayer;
		std::shared_ptr<graphics::OrthographicCamera> mCamera;
	};
}

#endif // _MAIN_SCENE_H