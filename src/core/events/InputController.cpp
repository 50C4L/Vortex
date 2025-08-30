#include "InputController.h"

#include <utility/Logger.h>

using namespace events;
using namespace utility;

InputController::InputController( std::unordered_map<SDL_Keycode, uint64_t>&& keys_to_event_ids )
	: mKeysToEventIds( std::move( keys_to_event_ids ) )
{
}

InputController::~InputController()
{
}

void
InputController::Handle( SDL_Event& event )
{
	if( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP )
	{
		bool is_down = event.type == SDL_KEYDOWN;
		auto it = mKeysToEventIds.find( event.key.keysym.sym );
		if( it != mKeysToEventIds.end() )
		{
			// Send event
			uint64_t event_id = it->second;
			auto obs_it = mEventObservers.find( event_id );
			if( obs_it != mEventObservers.end() )
			{
				for( auto observer : obs_it->second )
				{
					observer->OnInputEvent( event_id, is_down );
				}
			}
		}
	}
}

void
InputController::Subscribe( uint64_t event_id, Observer* observer )
{
	auto it = mEventObservers.find( event_id );
	if( it == mEventObservers.end() )
	{
		mEventObservers[event_id] = std::vector<Observer*>{ observer };
		return;
	}
	mEventObservers[event_id].push_back( observer );
}

void
InputController::Unsubscribe( uint64_t event_id, Observer* observer )
{
	auto it = mEventObservers.find( event_id );
	if( it != mEventObservers.end() )
	{
		auto& observers = it->second;
		observers.erase( std::remove( observers.begin(), observers.end(), observer ), observers.end() );
		if( observers.empty() )
		{
			mEventObservers.erase( it );
		}
	}
}
