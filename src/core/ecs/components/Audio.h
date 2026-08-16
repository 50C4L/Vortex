#ifndef _EAGE_COMPONENTS_AUDIO_H_
#define _EAGE_COMPONENTS_AUDIO_H_

#include <ecs/ResourceStore.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace eage::ecs
{

// Stores named audio sources for an entity.
// Each source is a ResourceId referencing a SoundPool in AudioSystem.
// An entity may own multiple named sources (e.g. "thruster", "weapon_fire").
struct AudioSourceComponent
{
	struct Source
	{
		ResourceId sound_id = 0;
	};
	std::unordered_map<std::string, Source> sources;
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

	struct Event
	{
		std::string source_name; // Key into AudioSourceComponent::sources
		EventType type;
	};

	std::vector<Event> pending_events;

	void QueueEvent( const std::string& source_name, EventType type )
	{
		pending_events.push_back( { source_name, type } );
	}
};

}

#endif // _EAGE_COMPONENTS_AUDIO_H_