#include "SceneResourceLoader.h"

#include <ecs/systems/AnimationSystem.h>
#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <utility/JsonParser.h>
#include <utility/Logger.h>

using namespace assets;
using namespace eage::ecs;
using namespace utility;

SceneResourceLoader::SceneResourceLoader( RenderSystem& render_system,
											AnimationSystem& animation_system,
											AudioSystem& audio_system )
	: mRenderSystem( render_system )
	, mAnimationSystem( animation_system )
	, mAudioSystem( audio_system )
{
}

bool
SceneResourceLoader::LoadManifest( const std::string& manifest_path )
{
	rapidjson::Document document;
	if( !parse_json_document( document, manifest_path ) )
	{
		LOG_ERROR() << "SceneResourceLoader: failed to parse manifest " << manifest_path;
		return false;
	}

	if( document.HasMember( "textures" ) && document["textures"].IsArray() )
	{
		for( const auto& texture_val : document["textures"].GetArray() )
		{
			if( !texture_val.IsString() )
			{
				LOG_ERROR() << "SceneResourceLoader: texture entry must be a string path";
				continue;
			}

			const std::string path = texture_val.GetString();
			try
			{
				const uint32_t texture_index = mRenderSystem.CreateTexture( path );
				mTextures[path] = texture_index;
			}
			catch( const std::exception& ex )
			{
				LOG_ERROR() << "SceneResourceLoader: failed to load texture " << path << ": " << ex.what();
			}
		}
	}

	if( document.HasMember( "animations" ) && document["animations"].IsArray() )
	{
		for( const auto& animation_val : document["animations"].GetArray() )
		{
			if( !animation_val.IsString() )
			{
				LOG_ERROR() << "SceneResourceLoader: animation entry must be a string path";
				continue;
			}

			const std::string path = animation_val.GetString();
			const ResourceId clip_id = mAnimationSystem.LoadClip( mRenderSystem, path );
			if( clip_id == INVALID_ID )
			{
				LOG_ERROR() << "SceneResourceLoader: failed to load animation clip " << path;
				continue;
			}

			mClips[path] = clip_id;
		}
	}

	if( document.HasMember( "sounds" ) && document["sounds"].IsArray() )
	{
		for( const auto& sound_val : document["sounds"].GetArray() )
		{
			if( !sound_val.IsObject() )
			{
				LOG_ERROR() << "SceneResourceLoader: sound entry must be an object";
				continue;
			}

			const std::string path = get_json_string( sound_val, "path" );
			if( path.empty() )
			{
				LOG_ERROR() << "SceneResourceLoader: sound entry missing path";
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
				LOG_ERROR() << "SceneResourceLoader: failed to load sound " << path;
				continue;
			}

			mSounds[path] = sound_id;
		}
	}

	return true;
}

uint32_t
SceneResourceLoader::GetTexture( const std::string& path ) const
{
	auto it = mTextures.find( path );
	if( it == mTextures.end() )
	{
		LOG_ERROR() << "SceneResourceLoader: texture not in catalog: " << path;
		return 0;
	}

	return it->second;
}

ResourceId
SceneResourceLoader::GetClip( const std::string& path ) const
{
	auto it = mClips.find( path );
	if( it == mClips.end() )
	{
		LOG_ERROR() << "SceneResourceLoader: animation clip not in catalog: " << path;
		return INVALID_ID;
	}

	return it->second;
}

ResourceId
SceneResourceLoader::GetSound( const std::string& path ) const
{
	auto it = mSounds.find( path );
	if( it == mSounds.end() )
	{
		LOG_ERROR() << "SceneResourceLoader: sound not in catalog: " << path;
		return INVALID_ID;
	}

	return it->second;
}
