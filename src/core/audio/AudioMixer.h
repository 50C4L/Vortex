#ifndef _EAGE_AUDIO_MIXER_H_
#define _EAGE_AUDIO_MIXER_H_

#include <memory>
#include <functional>

struct ma_engine;

namespace audio
{
	class AudioMixer
	{
	public:
		AudioMixer();
		~AudioMixer();

	private:
		std::unique_ptr<ma_engine, std::function<void(ma_engine*)>> mEngine;
	};
}

#endif // _EAGE_AUDIO_MIXER_H_
