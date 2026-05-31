#ifndef _EAGE_INPUT_CONTROLLER_H
#define _EAGE_INPUT_CONTROLLER_H

#include "KeyCode.h"

#include <unordered_map>
#include <functional>

union SDL_Event;

namespace events
{
	///
	/// InputController: Maps SDL input events to game event IDs and notifies observers
	///
	class InputController
	{
	public:
		class Observer
		{
		public:
			virtual void OnInputEvent( uint64_t event_id, bool on ) = 0;
		};

		/// Constructor takes a mapping of KeyCode to event IDs
		///
		/// @param keys_to_event_ids
		///   Map of KeyCode to event IDs
		InputController( std::unordered_map<KeyCode, uint64_t>&& keys_to_event_ids );
		~InputController();

		/// Handle an SDL_Event, notify observers if mapped
		void Handle( SDL_Event& event );

		/// Subscribe an observer to a specific event ID
		void Subscribe( uint64_t event_id, Observer* observer );

		/// Unsubscribe an observer from a specific event ID
		void Unsubscribe( uint64_t event_id, Observer* observer );

	private:
		std::unordered_map<KeyCode, uint64_t> mKeysToEventIds;
		std::unordered_map<uint64_t, std::vector<Observer*>> mEventObservers;
	};
}

#endif