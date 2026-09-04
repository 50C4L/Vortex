#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"
#include "../EngineContext.h"
#include <ecs/ECS.h>
#include <ecs/ResourceStore.h>
#include <events/InputController.h>
#include <utility/DeltaTimer.h>

#include <memory>

namespace eage::graphics
{
	class OrthographicCamera;
	class Renderer;
	class SceneRenderPass;
	class CompositePass;
}

namespace eage::ui
{
	class UISystem;
	class UIView;
}

namespace eage::ecs
{
	class AnimationSystem;
	class AudioSystem;
	class EffectSystem;
	class SceneGraphSystem;
}

namespace assets
{
	class SceneResourceLoader;
}

namespace vortex
{
	class EnemySystem;
	class BulletSystem;
	class LevelingSystem;
	class PlayerInputSystem;
	class PlayerGameplaySystem;
	class StatusPanel;
	class WarpSystem;
	class WaveStore;
	class WaveSystem;

	class MainScene : public AbstractScene, public events::InputController::Observer
	{
	public:
		explicit MainScene( const EngineContext& ctx );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual uint64_t GetSceneRoot() override;
		virtual void OnExit() override;
		virtual void Update( float dt ) override;

		virtual eage::graphics::ManagedImage* GetOutput() override;

		void Pause( bool paused );

		// events::InputController::Observer
		void OnInputEvent( uint64_t event_id, bool on ) override;

	private:
		void InitializeGenericSystems();
		void PrepareMeshes();
		void PrepareMaterials();
		void CreateSceneRoot();
		void CreateBackgroundEntity();
		void CreatePlayerEntity();
		void CreateScreenZoneEntities();
		void CreateEnemyEntities();
		void CreateExplosionEffect();

		eage::graphics::Renderer& mRenderer;
		eage::ui::UISystem& mUISystem;
		events::InputController& mInputController;
		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::AudioSystem& mAudioSystem;
		eage::ecs::AnimationSystem& mAnimationSystem;
		eage::ecs::EffectSystem& mEffectSystem;
		eage::ecs::RenderSystem& mRenderSystem;
		eage::ecs::PhysicsSystem& mPhysicsSystem;
		eage::ecs::SceneGraphSystem& mSceneGraphSystem;
		assets::SceneResourceLoader& mResourceLoader;

		std::unique_ptr<eage::graphics::SceneRenderPass> mScenePass;
		std::unique_ptr<eage::ui::UIView> mUIView;
		std::unique_ptr<eage::graphics::CompositePass> mCompositePass;
		std::unique_ptr<StatusPanel> mStatusPanel;
		std::unique_ptr<PlayerInputSystem> mPlayerInputSystem;
		std::unique_ptr<PlayerGameplaySystem> mPlayerGameplaySystem;
		std::unique_ptr<WarpSystem> mWarpSystem;
		std::unique_ptr<EnemySystem> mEnemySystem;
		std::unique_ptr<WaveStore> mWaveStore;
		std::unique_ptr<WaveSystem> mWaveSystem;
		std::unique_ptr<BulletSystem> mBulletSystem;
		std::unique_ptr<LevelingSystem> mLevelingSystem;

		std::shared_ptr<eage::graphics::OrthographicCamera> mCamera;

		uint64_t mSceneRootEntity = 0;
		uint64_t mOnScreenZoneEntity = 0;
		uint64_t mBackgroundEntity = 0;
		eage::ecs::ResourceId mExplosionEffectId = 0;
		eage::ecs::ResourceHandle mEffectMaterial;

		utility::DeltaTimer mPlayerGameplayTimer;
		utility::DeltaTimer mBulletTimer;
		utility::DeltaTimer mWaveTimer;
		bool mPaused = false;
	};
}

#endif // _MAIN_SCENE_H
