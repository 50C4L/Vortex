#ifndef _EAGE_SYSTEMS_AUDIO_SYSTEM_H_
#define _EAGE_SYSTEMS_AUDIO_SYSTEM_H_

#include <ecs/components/Audio.h>
#include <ecs/ResourceStore.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace eage::audio
{
	class AudioMixer;
	class SoundInstance;
}

namespace eage::ecs
{
	class ECSRegistry;

	///
	/// AudioSystem: Manages audio resources and processes audio events from ECS components.
	///
	/// All sounds are loaded through a unified pool. Use pool_size > 1 for sounds that
	/// need to overlap (e.g. rapid-fire SFX). Use looping = true for sustained sounds
	/// (e.g. engine hum). Play() and Stop() work for both cases.
	///
	class AudioSystem
	{
	public:
		struct SoundConfig
		{
			std::string path;
			int pool_size = 1;    // Number of simultaneous instances; >1 enables overlap via round-robin
			bool looping = false;
		};

		AudioSystem( eage::ecs::ECSRegistry& registry, audio::AudioMixer& audio_mixer );

		void Update( float delta_time );

		// Load a sound pool and return its ResourceId, or existing Id if already loaded.
		eage::ecs::ResourceId LoadSound( const SoundConfig& config );

		// Play the next available instance in the pool.
		// Looping pools: starts the single instance if not already playing.
		// One-shot pools: restarts the next instance in round-robin order.
		void Play( eage::ecs::ResourceId sound_id );

		// Stop all instances in the pool.
		void Stop( eage::ecs::ResourceId sound_id );

	private:
		struct SoundPool
		{
			std::vector<std::unique_ptr<audio::SoundInstance>> instances;
			int next_index = 0;
			bool looping = false;
			bool is_playing = false;
		};

		void ProcessAudioEvents();

		eage::ecs::ECSRegistry& mRegistry;
		audio::AudioMixer& mAudioMixer;
		std::unordered_map<eage::ecs::ResourceId, SoundPool> mSoundPools;
		std::unordered_map<std::string, eage::ecs::ResourceId> mSoundPathToId;
		eage::ecs::ResourceId mNextSoundId = 1;
	};
}

#endif // _EAGE_SYSTEMS_AUDIO_SYSTEM_H_