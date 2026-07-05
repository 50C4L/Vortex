#include "AnimationClip.h"

#include <filesystem>

#include "TextureAtlas.h"
#include <utility/Logger.h>
#include <utility/JsonParser.h>

using namespace utility;
using namespace assets;


AnimationClip::AnimationClip( const std::string& clip_json_path )
{
	mDefaultFrame.uv_min = glm::vec2( 0.f, 0.f );
	mDefaultFrame.uv_max = glm::vec2( 1.f, 1.f );
	mDefaultFrame.duration_sec = 0.1f;

	const std::filesystem::path clip_path( clip_json_path );
	mClipDirectory = clip_path.parent_path().string();
	if( !mClipDirectory.empty() && mClipDirectory.back() != '/' && mClipDirectory.back() != '\\' )
	{
		mClipDirectory += '/';
	}

	rapidjson::Document document;
	if( !parse_json_document( document, clip_json_path ) )
	{
		return;
	}

	bool flip = false;
	if( document.HasMember( "flip" ) && document["flip"].IsBool() )
	{
		flip = document["flip"].GetBool();
	}

	if( !document.HasMember( "frames" ) || !document["frames"].IsArray() )
	{
		LOG_ERROR( "AnimationClip: missing 'frames' array in " + clip_json_path );
		return;
	}

	const bool has_atlas = document.HasMember( "atlas" ) && document["atlas"].IsString();
	if( has_atlas )
	{
		std::string atlas_path = get_json_string( document, "atlas" );
		if( atlas_path.empty() )
		{
			LOG_ERROR( "AnimationClip: missing 'atlas' field in " + clip_json_path );
			return;
		}

		TextureAtlas atlas( atlas_path );
		if( flip )
		{
			atlas.Flip();
		}

		for( const auto& frame_val : document["frames"].GetArray() )
		{
			std::string name = get_json_string( frame_val, "name" );
			int duration_ms = get_json_int( frame_val, "duration_ms" );

			const auto& sub_tex = atlas.GetSubTexture( name );

			Frame frame;
			frame.uv_min = sub_tex.uv_min;
			frame.uv_max = sub_tex.uv_max;
			frame.duration_sec = static_cast<float>( duration_ms ) / 1000.f;
			mFrames.push_back( frame );
		}

		return;
	}

	mUsesPerFrameTextures = true;

	for( const auto& frame_val : document["frames"].GetArray() )
	{
		std::string texture_path = get_json_string( frame_val, "texture" );
		int duration_ms = get_json_int( frame_val, "duration_ms" );
		if( texture_path.empty() )
		{
			LOG_ERROR( "AnimationClip: frame missing 'texture' in " + clip_json_path );
			continue;
		}

		Frame frame;
		frame.uv_min = glm::vec2( 0.f, 0.f );
		frame.uv_max = glm::vec2( 1.f, 1.f );
		frame.duration_sec = static_cast<float>( duration_ms ) / 1000.f;
		frame.texture_path = texture_path;
		mFrames.push_back( frame );
	}
}

AnimationClip::~AnimationClip() = default;

const AnimationClip::Frame&
AnimationClip::GetFrame( int index ) const
{
	if( index < 0 || index >= static_cast<int>( mFrames.size() ) )
	{
		return mDefaultFrame;
	}
	return mFrames[index];
}

int
AnimationClip::GetFrameCount() const
{
	return static_cast<int>( mFrames.size() );
}

bool
AnimationClip::UsesPerFrameTextures() const
{
	return mUsesPerFrameTextures;
}

const std::string&
AnimationClip::GetClipDirectory() const
{
	return mClipDirectory;
}

std::string
AnimationClip::GetResolvedTexturePath( int index ) const
{
	const Frame& frame = GetFrame( index );
	if( frame.texture_path.empty() )
	{
		return {};
	}

	if( mClipDirectory.empty() )
	{
		return frame.texture_path;
	}

	return mClipDirectory + frame.texture_path;
}
