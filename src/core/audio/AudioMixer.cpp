#include "AudioMixer.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

// #include <utility/Logger.h>
#include <utility/Pointers.h>

using namespace audio;

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
		//LOG_ERROR( "Failed to initialize audio engine." );
		throw std::runtime_error( "Failed to initialize audio engine." );
	}
}

AudioMixer::~AudioMixer()
{
}
