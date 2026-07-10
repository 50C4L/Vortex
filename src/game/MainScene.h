#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"
#include "../EngineContext.h"
#include <assets/AnimationClip.h>
#include <ecs/ResourceManager.h>
#include <ecs/ECS.h>

#include <memory>

namespace eage::graphics
{
	class OrthographicCamera;
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
		explicit MainScene( const EngineContext& ctx );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual uint64_t GetSceneRoot() override;
		virtual void OnExit() override;
		virtual void Update( float dt ) override;

	private:
		void InitializeGenericSystems();
		void PrepareAnimations();
		void PrepareMeshes();
		void PrepareMaterials();
		void CreateSceneRoot();
		void CreateBackgroundEntity();
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

		std::shared_ptr<const assets::AnimationClip> mDefaultBulletClip;
		std::unique_ptr<BulletSystem> mBulletSystem;

		std::shared_ptr<eage::graphics::OrthographicCamera> mCamera;

		uint64_t mSceneRootEntity = 0;
		uint64_t mOnScreenZoneEntity = 0;
		uint64_t mBackgroundEntity = 0;
		eage::ecs::Entity mKillCountHudEntity = 0;
	};
}

#endif // _MAIN_SCENE_H