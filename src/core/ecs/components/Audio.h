#ifndef _EAGE_COMPONENTS_AUDIO_H_
#define _EAGE_COMPONENTS_AUDIO_H_

#include <string>
#include <vector>

struct AudioSourceComponent 
{
	std::string sound_path;  // Path to sound file
	uint32_t sound_resource_id = 0;  // Managed by AudioSystem
	float volume = 1.0f;
	bool should_loop = false;
	bool is_playing = false;
};

struct AudioEventComponent 
{
	enum class EventType
	{
		Play,
		Stop,
		Pause,
		Resume
	};

	std::vector<EventType> pending_events;  // Queue of events to process

	void QueueEvent( EventType event )
	{
		pending_events.push_back( event );
	}
};

#endif // _EAGE_COMPONENTS_AUDIO_H_