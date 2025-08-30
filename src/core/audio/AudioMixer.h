#ifndef _EAGE_AUDIO_MIXER_H_
#define _EAGE_AUDIO_MIXER_H_

#include <memory>
#include <functional>
#include <string>

struct ma_engine;
struct ma_sound;

namespace eage::audio
{
	///
	/// Instance of a sound file, it's loaded into memory asynchroniously.
	/// Use `AudioMixer::CreateSound` to create an instance of a sound file.
	/// Each instance is expected to be played once at a time, multiple instances
	/// are expected to be created for multiple simultaneous sounds.
	///
	class SoundInstance
	{
	public:
		SoundInstance( ma_engine& engine, const std::string sound_path );
		~SoundInstance();

		SoundInstance( const SoundInstance& ) = delete;
		SoundInstance& operator=( const SoundInstance& ) = delete;

		SoundInstance( SoundInstance&& ) = default;
		SoundInstance& operator=( SoundInstance&& ) = default;

		void Play();
		void Stop();

	private:
		ma_engine& mEngine;
		std::unique_ptr<ma_sound, std::function<void(ma_sound*)>> mSound;
	};

	class AudioMixer
	{
	public:
		AudioMixer();
		~AudioMixer();

		SoundInstance CreateSound( const std::string sound_path );

	private:
		std::unique_ptr<ma_engine, std::function<void(ma_engine*)>> mEngine;
	};
}

#endif // _EAGE_AUDIO_MIXER_H_
