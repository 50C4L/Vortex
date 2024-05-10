#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <chrono>

#include "GameMaterials.h"

namespace graphics
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

namespace vortex
{
	class Ship;

	class MainScene : public AbstractScene
	{
	public:
		MainScene( graphics::Renderer& renderer );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual void OnExit() override;

		virtual void Update() override;

		void PrepareMeshes();
		void PrepareMaterials();

	private:
		graphics::Renderer& mRenderer;
		vk::UniqueDescriptorSetLayout mSceneGlobalDataLayout;
		std::shared_ptr<graphics::UniformDescriptor> mSceneGlobalDescriptor;
		std::unique_ptr<graphics::ManagedBuffer, std::function<void(graphics::ManagedBuffer*)>> mSceneGlobalDataDynamic;

		std::shared_ptr<graphics::OrthographicCamera> mCamera;

		std::unique_ptr<SingleTextureSpriteMaterial> mSpriteMaterial;
		std::unique_ptr<SingleTextureSpriteMaterial::Resources> mSpriteMaterialResources;

		std::shared_ptr<graphics::GPUMeshBuffers> mQuadMesh;
		std::unique_ptr<Ship> mShip;

		std::chrono::time_point<std::chrono::high_resolution_clock> mLastUpdateTime;
	};
}

#endif // _MAIN_SCENE_H