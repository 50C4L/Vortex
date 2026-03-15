#include "PlayerInputSystem.h"

#include "../components/PlayerComponents.h"
#include "../GameConfig.h"

using namespace vortex;
using namespace vortex::config;

PlayerInputSystem::PlayerInputSystem( eage::ecs::ECSRegistry& registry, events::InputController& input_controller )
	: mRegistry(registry)
	, mInputController(input_controller)
{
	// Register for the events we care about
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_THRUST ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_SHOOT ), this );
}

PlayerInputSystem::~PlayerInputSystem() 
{
	mInputController.Unsubscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ), this );
	mInputController.Unsubscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ), this );
	mInputController.Unsubscribe( static_cast<uint64_t>( GameEvents::PLAYER_THRUST ), this );
	mInputController.Unsubscribe( static_cast<uint64_t>( GameEvents::PLAYER_SHOOT ), this );
}

void
PlayerInputSystem::OnInputEvent( uint64_t event_id, bool on )
{
	auto event = static_cast<GameEvents>( event_id );
	// Update player component based on events
	for( auto& [entity, player] : mRegistry.GetComponentMap<PlayerComponent>() )
	{
		switch( event ) 
		{
			case GameEvents::PLAYER_ROTATE_LEFT:
				player.turning_left = on;
				break;
			case GameEvents::PLAYER_ROTATE_RIGHT:
				player.turning_right = on;
				break;
			case GameEvents::PLAYER_THRUST:
				player.thruster_on = on;
				break;
			case GameEvents::PLAYER_SHOOT:
				player.main_weapon_firing = on;
				break;
			default:
				break;
		}
	}
}
