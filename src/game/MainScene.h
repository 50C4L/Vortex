#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"
#include <ecs/ResourceManager.h>
#include <ecs/ECS.h>

#include <memory>
#include <chrono>

namespace eage::graphics
{
	class OrthographicCamera;
}

namespace events
{
	class InputController;
}

namespace eage::ecs
{
	class AudioSystem;
	class ECSRegistry;
	class PhysicsSystem;
	class RenderSystem;
}

namespace vortex
{
	class AsteroidGameplaySystem;
	class AsteroidGameplaySystem;
	class BulletSystem;
	class PlayerInputSystem;
	class PlayerGameplaySystem;
	class WarpSystem;

	class MainScene : public AbstractScene
	{
	public:
		MainScene( events::InputController& input_controller, 
				   eage::ecs::ECSRegistry& ecs_registry, eage::ecs::AudioSystem& audio_system, 
				   eage::ecs::RenderSystem& render_system, eage::ecs::PhysicsSystem& physics_system );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual uint64_t GetSceneRoot() override;
		virtual void OnExit() override;
		virtual void Update() override;

	private:
		void InitializeGenericSystems();
		void PrepareMeshes();
		void PrepareMaterials();
		void CreateSceneRoot();
		void CreatePlayerEntity();
		void CreateScreenZoneEntities();
		void CreateEnemyEntities();
		void CreateHudEntities();

		events::InputController& mInputController;
		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::AudioSystem& mAudioSystem;
		eage::ecs::RenderSystem& mRenderSystem;
		eage::ecs::PhysicsSystem& mPhysicsSystem;

		std::unique_ptr<PlayerInputSystem> mPlayerInputSystem;
		std::unique_ptr<PlayerGameplaySystem> mPlayerGameplaySystem;
		std::unique_ptr<WarpSystem> mWarpSystem;
		std::unique_ptr<AsteroidGameplaySystem> mAsteroidGameplaySystem;
		std::unique_ptr<BulletSystem> mBulletSystem;

		std::shared_ptr<eage::graphics::OrthographicCamera> mCamera;

		uint64_t mSceneRootEntity = 0;
		uint64_t mOnScreenZoneEntity = 0;
		eage::ecs::Entity mKillCountHudEntity = 0;

		std::chrono::time_point<std::chrono::steady_clock> mLastUpdateTime;
	};
}

#endif // _MAIN_SCENE_H