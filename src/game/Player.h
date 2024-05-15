#ifndef _VORTEX_PLAYER_H
#define _VORTEX_PLAYER_H

#include <memory>
#include <chrono>

#include <events/InputController.h>

namespace graphics
{
	class Renderer;
	struct GPUMeshBuffers;
	struct Material;
}

namespace vortex
{
	class Ship;

	class Player : public events::InputController::Observer
	{
	public:
		Player( graphics::Renderer& renderer, events::InputController& input_controller );
		virtual ~Player();

		void Init( std::shared_ptr<graphics::GPUMeshBuffers> mesh_buffer, std::shared_ptr<graphics::Material> material );

		void Update();

		void Draw();

		virtual void OnInputEvent( uint64_t event_id, bool on ) override;

	private:
		void StopRotation();

		graphics::Renderer& mRenderer;
		events::InputController& mInputController;
		std::unique_ptr<Ship> mShip;

		struct RotateState
		{
			bool left = false;
			bool right = false;
		} mRotateState;

		std::chrono::time_point<std::chrono::steady_clock> mLastUpdateTime;
	};
}

#endif