#ifndef _EAGE_INPUT_CONTROLLER_H
#define _EAGE_INPUT_CONTROLLER_H

#include <SDL2/SDL.h>

#include <unordered_map>
#include <functional>

namespace events
{
	class InputController
	{
	public:
		class Observer
		{
		public:
			virtual void OnInputEvent( uint64_t event_id, bool on ) = 0;
		};

		InputController( std::unordered_map<SDL_Keycode, uint64_t>&& keys_to_event_ids );
		~InputController();

		void Handle( SDL_Event& event );

		void Subscribe( uint64_t event_id, Observer* observer );
		void Unsubscribe( uint64_t event_id, Observer* observer );

	private:
		std::unordered_map<SDL_Keycode, uint64_t> mKeysToEventIds;
		std::unordered_map<uint64_t, std::vector<Observer*>> mEventObservers;
	};
}

#endif