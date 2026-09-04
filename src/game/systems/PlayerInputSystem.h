#ifndef VORTEX_PLAYER_INPUT_SYSTEM_H
#define VORTEX_PLAYER_INPUT_SYSTEM_H

#include <events/InputController.h>
#include <ecs/ECS.h>
#include <utility/Pausable.h>

namespace vortex
{
	///
	/// PlayerInputSystem: Maps input events to player actions and updates ECS components
	///
	class PlayerInputSystem final : public events::InputController::Observer, public utility::Pausable
	{
	public:
		PlayerInputSystem( eage::ecs::ECSRegistry& registry, events::InputController& input_controller );
		
		~PlayerInputSystem();
		
		void OnInputEvent( uint64_t event_id, bool on ) override;

	protected:
		void OnPauseChanged( bool paused ) override;
		
	private:
		eage::ecs::ECSRegistry& mRegistry;
		events::InputController& mInputController;
	};
} // namespace vortex


#endif // VORTEX_PLAYER_INPUT_SYSTEM_H
