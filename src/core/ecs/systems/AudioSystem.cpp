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
AudioSystem::LoadSound( const SoundConfig& config )
{
	auto it = mSoundPathToId.find( config.path );
	if( it != mSoundPathToId.end() )
	{
		return it->second;
	}

	SoundPool pool;
	pool.looping = config.looping;
	pool.instances.reserve( config.pool_size );
	for( int i = 0; i < config.pool_size; ++i )
	{
		pool.instances.push_back(
			std::make_unique<audio::SoundInstance>(
				mAudioMixer.CreateSound( config.path, config.looping ) ) );
	}

	ResourceId id = mNextSoundId++;
	mSoundPathToId[config.path] = id;
	mSoundPools[id] = std::move( pool );
	return id;
}

void
AudioSystem::Play( eage::ecs::ResourceId sound_id )
{
	auto it = mSoundPools.find( sound_id );
	if( it == mSoundPools.end() ) return;

	auto& pool = it->second;
	if( pool.looping )
	{
		if( !pool.is_playing )
		{
			pool.instances[0]->Play();
			pool.is_playing = true;
		}
	}
	else
	{
		pool.instances[pool.next_index]->Restart();
		pool.next_index = ( pool.next_index + 1 ) % static_cast<int>( pool.instances.size() );
	}
}

void
AudioSystem::Stop( eage::ecs::ResourceId sound_id )
{
	auto it = mSoundPools.find( sound_id );
	if( it == mSoundPools.end() ) return;

	auto& pool = it->second;
	for( auto& instance : pool.instances )
	{
		instance->Stop();
	}
	pool.is_playing = false;
}

void
AudioSystem::ProcessAudioEvents()
{
	for( auto& [entity, audio_event] : mRegistry.GetComponentMap<AudioEventComponent>() )
	{
		if( !mRegistry.HasComponent<AudioSourceComponent>( entity ) )
		{
			audio_event.pending_events.clear();
			continue;
		}

		auto& audio_source = mRegistry.GetComponent<AudioSourceComponent>( entity );

		for( const auto& event : audio_event.pending_events )
		{
			auto source_it = audio_source.sources.find( event.source_name );
			if( source_it == audio_source.sources.end() )
			{
				LOG_ERROR() << "AudioSourceComponent on entity " << entity
							<< " has no source named '" << event.source_name << "'";
				continue;
			}

			ResourceId sound_id = source_it->second.sound_id;
			switch( event.type )
			{
			case AudioEventComponent::EventType::Play:
				Play( sound_id );
				break;
			case AudioEventComponent::EventType::Stop:
				Stop( sound_id );
				break;
			default:
				break;
			}
		}

		audio_event.pending_events.clear();
	}
}


