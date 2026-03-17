#include "AudioMixer.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include <utility/Logger.h>

using namespace eage::audio;

SoundInstance::SoundInstance( ma_engine& engine, const std::string sound_path, bool looping )
	: mEngine( engine )
{
	ma_result result;
	mSound = std::unique_ptr<ma_sound, std::function<void( ma_sound* )>>( 
		new ma_sound(),
		[]( ma_sound* sound )
		{ 
			ma_sound_uninit( sound ); 
		} );

	ma_sound_config config = ma_sound_config_init_2( &mEngine );
	config.pFilePath = sound_path.c_str();
	config.flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC;
	config.isLooping = looping ? MA_TRUE : MA_FALSE;
	config.pInitialAttachment = NULL;
	config.pDoneFence = NULL;

	result = ma_sound_init_ex( 
		&mEngine, &config,
		mSound.get() );

	if( result != MA_SUCCESS )
	{
		utility::LOG_ERROR( "Failed to load sound file: " + sound_path );
	}
}

SoundInstance::~SoundInstance()
{
	Stop();
}

void
SoundInstance::Play()
{
	ma_sound_start( mSound.get() );
}

void
SoundInstance::Stop()
{
	ma_sound_stop( mSound.get() );
}

void
SoundInstance::Restart()
{
	ma_sound_seek_to_pcm_frame( mSound.get(), 0 );
	ma_sound_start( mSound.get() );
}

AudioMixer::AudioMixer()
{
	ma_result result;
	mEngine = std::unique_ptr<ma_engine, std::function<void( ma_engine* )>>( 
	new ma_engine(),
	[]( ma_engine* engine )
	{ 
		ma_engine_uninit( engine ); 
	} );

	result = ma_engine_init( NULL, mEngine.get() );
	if( result != MA_SUCCESS )
	{
		utility::LOG_ERROR( "Failed to initialize audio engine." );
		throw std::runtime_error( "Failed to initialize audio engine." );
	}
}

AudioMixer::~AudioMixer()
{
}

SoundInstance 
AudioMixer::CreateSound( const std::string sound_path, bool looping )
{
	return SoundInstance( *mEngine.get(), sound_path, looping );
}
