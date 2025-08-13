#ifndef _VORTEX_PLAYER_H
#define _VORTEX_PLAYER_H

#include <memory>
#include <chrono>

#include <events/InputController.h>
#include <ecs/ECS.h>

#include "GameMaterials.h"

namespace eage::graphics
{
	class Renderer;
	struct GPUMeshBuffers;
	struct Material;
}

namespace audio
{
	class SoundInstance;
}

namespace eage::ecs
{
	class RenderSystem;
}

namespace vortex
{
	class Ship;
	class ShipControlSystem;

	class Player : public events::InputController::Observer
	{
	public:
		Player( eage::graphics::Renderer& renderer, events::InputController& input_controller, eage::ecs::ECSRegistry& ecs_registry, 
				eage::ecs::RenderSystem& render_system );
		virtual ~Player();

		void Init( SingleTextureSpriteMaterial& material,
				   SingleTextureSpriteMaterial::Resources& resources,
				   std::unique_ptr<audio::SoundInstance> engine_sound );

		void Update();

		void Draw();

		virtual void OnInputEvent( uint64_t event_id, bool on ) override;

	private:
		void StopRotation();

		eage::graphics::Renderer& mRenderer;
		events::InputController& mInputController;
		eage::ecs::ECSRegistry& mEcsRegistry;
		eage::ecs::RenderSystem& mRenderSystem;
		eage::ecs::Entity mShipEntity;
		std::unique_ptr<Ship> mShip;
		std::unique_ptr<ShipControlSystem> mShipControlSystem;
		std::unique_ptr<audio::SoundInstance> mEngineSound;

		struct RotateState
		{
			bool left = false;
			bool right = false;
		} mRotateState;

		std::chrono::time_point<std::chrono::steady_clock> mLastUpdateTime;
	};
}

#endif