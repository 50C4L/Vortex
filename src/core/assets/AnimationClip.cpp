#include "AnimationClip.h"

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

	rapidjson::Document document;
	if( !parse_json_document( document, clip_json_path ) )
	{
		return;
	}

	std::string atlas_path = get_json_string( document, "atlas" );
	if( atlas_path.empty() )
	{
		LOG_ERROR( "AnimationClip: missing 'atlas' field in " + clip_json_path );
		return;
	}

	bool flip = false;
	if( document.HasMember( "flip" ) && document["flip"].IsBool() )
	{
		flip = document["flip"].GetBool();
	}

	TextureAtlas atlas( atlas_path );
	if( flip )
	{
		atlas.Flip();
	}

	if( !document.HasMember( "frames" ) || !document["frames"].IsArray() )
	{
		LOG_ERROR( "AnimationClip: missing 'frames' array in " + clip_json_path );
		return;
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
