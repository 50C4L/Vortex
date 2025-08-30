#ifndef _EAGE_SYSTEMS_AUDIO_SYSTEM_H_
#define _EAGE_SYSTEMS_AUDIO_SYSTEM_H_

#include <ecs/components/Audio.h>
#include <ecs/ResourceManager.h>
#include <memory>
#include <unordered_map>

namespace eage::audio
{
	class AudioMixer;
	class SoundInstance;
}

namespace eage::ecs
{
	class ECSRegistry;

	class AudioSystem
	{
	public:
		AudioSystem( eage::ecs::ECSRegistry& registry, audio::AudioMixer& audio_mixer );
		
		void Update( float delta_time );

		eage::ecs::ResourceId LoadSound( const std::string& sound_path );
		
	private:
		void ProcessAudioEvents();
		
		void ProcessAudioEvent( AudioSourceComponent& audio_source, AudioEventComponent::EventType event);
	
		eage::ecs::ECSRegistry& mRegistry;
		audio::AudioMixer& mAudioMixer;
		ResourceManager<std::unique_ptr<audio::SoundInstance>> mSounds;
		std::unordered_map<std::string, eage::ecs::ResourceId> mSoundPathToId;
	};
}

#endif // _EAGE_SYSTEMS_AUDIO_SYSTEM_H_