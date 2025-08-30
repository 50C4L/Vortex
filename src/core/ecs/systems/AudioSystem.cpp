#include "AudioSystem.h"

#include <audio/AudioMixer.h>
#include <ecs/ECS.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace utility;

AudioSystem::AudioSystem( eage::ecs::ECSRegistry& registry, audio::AudioMixer& audio_mixer )
	: mRegistry(registry)
	, mAudioMixer(audio_mixer)
{
}

void
AudioSystem::Update( float delta_time )
{
	ProcessAudioEvents();
}

eage::ecs::ResourceId
AudioSystem::LoadSound( const std::string& sound_path )
{
	// Check if sound already loaded
	auto it = mSoundPathToId.find( sound_path );
	if( it != mSoundPathToId.end() )
	{
		return it->second;
	}

	// Load new sound
	auto sound_instance = std::make_unique<audio::SoundInstance>( mAudioMixer.CreateSound( sound_path ) );
	if( !sound_instance ) 
	{
		return INVALID_ID;
	}

	ResourceId id = mSounds.Store( std::move( sound_instance ) );
	if( id != INVALID_ID )
	{
		mSoundPathToId[sound_path] = id;
	}
	return id;
}

void
AudioSystem::ProcessAudioEvents()
{
	// Iterate over all entities with AudioEventComponent
	for( auto& [entity, audio_event] : mRegistry.GetComponentMap<AudioEventComponent>() )
	{
		// Check if entity has AudioSourceComponent
		if( mRegistry.HasComponent<AudioSourceComponent>( entity ) )
		{
			auto& audio_source = mRegistry.GetComponent<AudioSourceComponent>( entity );
			
			// Process each event
			for( const auto& event : audio_event.pending_events )
			{
				ProcessAudioEvent( audio_source, event );
			}
			
			// Clear events after processing
			audio_event.pending_events.clear();
		}
	}
}

void
AudioSystem::ProcessAudioEvent( AudioSourceComponent& audio_source, AudioEventComponent::EventType event )
{
	switch( event )
	{
	case AudioEventComponent::EventType::Play:
		if( !audio_source.is_playing )
		{
			// Load sound if not already loaded
			if( audio_source.sound_resource_id == INVALID_ID )
			{
				LOG() << "Sound resource ID is invalid: " << audio_source.sound_path;
				return;
			}

			if( audio_source.sound_resource_id != 0 )
			{
				if( auto sound_instance = mSounds.Get( audio_source.sound_resource_id ) )
				{
					sound_instance->Play();
					audio_source.is_playing = true;
				}
			}
		}
		break;
	case AudioEventComponent::EventType::Stop:
		if( audio_source.is_playing && audio_source.sound_resource_id != 0 )
		{
			if( auto sound_instance = mSounds.Get( audio_source.sound_resource_id ) )
			{
				sound_instance->Stop();
				audio_source.is_playing = false;
			}
		}
		break;
	case AudioEventComponent::EventType::Pause:
		// Pause functionality can be implemented if supported by SoundInstance
		break;
	case AudioEventComponent::EventType::Resume:
		// Resume functionality can be implemented if supported by SoundInstance
		break;
	default:
		break;
	}
}

