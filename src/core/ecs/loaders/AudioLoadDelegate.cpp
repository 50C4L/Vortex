#include "AudioLoadDelegate.h"

#include <ecs/systems/AudioSystem.h>
#include <utility/JsonParser.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace utility;

AudioLoadDelegate::AudioLoadDelegate( AudioSystem& audio_system )
	: mAudioSystem( audio_system )
{
}

bool
AudioLoadDelegate::Load( const rapidjson::Value& section,
						 assets::SceneResourceLoader::ResourceTable& out )
{
	if( !section.IsArray() )
	{
		LOG_ERROR() << "AudioLoadDelegate: section must be an array";
		return false;
	}

	bool ok = true;
	for( const auto& sound_val : section.GetArray() )
	{
		if( !sound_val.IsObject() )
		{
			LOG_ERROR() << "AudioLoadDelegate: sound entry must be an object";
			ok = false;
			continue;
		}

		const std::string path = get_json_string( sound_val, "path" );
		if( path.empty() )
		{
			LOG_ERROR() << "AudioLoadDelegate: sound entry missing path";
			ok = false;
			continue;
		}

		AudioSystem::SoundConfig config;
		config.path = path;
		config.pool_size = get_json_int( sound_val, "pool_size" );
		if( config.pool_size <= 0 )
		{
			config.pool_size = 1;
		}

		if( sound_val.HasMember( "looping" ) && sound_val["looping"].IsBool() )
		{
			config.looping = sound_val["looping"].GetBool();
		}

		const ResourceId sound_id = mAudioSystem.LoadSound( config );
		if( sound_id == INVALID_ID )
		{
			LOG_ERROR() << "AudioLoadDelegate: failed to load sound " << path;
			ok = false;
			continue;
		}

		out[path] = sound_id;
	}

	return ok;
}
