#include "AnimationClip.h"

#include <filesystem>

#include <assets/ImageLoader.h>
#include <ecs/systems/RenderSystem.h>
#include <utility/Logger.h>
#include <utility/JsonParser.h>

using namespace utility;
using namespace assets;


std::shared_ptr<AnimationClip>
AnimationClip::Load( eage::ecs::RenderSystem& render_system, const std::string& clip_json_path )
{
	auto clip = std::shared_ptr<AnimationClip>( new AnimationClip() );
	clip->mDefaultFrame.texture_index = 0;
	clip->mDefaultFrame.duration_sec = 0.1f;

	rapidjson::Document document;
	if( !parse_json_document( document, clip_json_path ) )
	{
		LOG_ERROR( "AnimationClip: failed to parse " + clip_json_path );
		return clip;
	}

	if( !document.HasMember( "frames" ) || !document["frames"].IsArray() )
	{
		LOG_ERROR( "AnimationClip: missing 'frames' array in " + clip_json_path );
		return clip;
	}

	const std::filesystem::path json_directory = std::filesystem::path( clip_json_path ).parent_path();

	for( const auto& frame_val : document["frames"].GetArray() )
	{
		std::string texture_name = get_json_string( frame_val, "texture" );
		if( texture_name.empty() )
		{
			LOG_ERROR( "AnimationClip: frame missing 'texture' in " + clip_json_path );
			continue;
		}

		int duration_ms = get_json_int( frame_val, "duration_ms" );
		const std::filesystem::path texture_path = json_directory / texture_name;

		if( clip->mFrames.empty() )
		{
			ImageLoader image_loader;
			const ImageLoader::Image image = image_loader.LoadImage( texture_path.string() );
			if( image.width > 0 && image.height > 0 )
			{
				clip->mFrameWidth = image.width;
				clip->mFrameHeight = image.height;
			}
		}

		Frame frame;
		frame.texture_index = render_system.CreateTexture( texture_path.string() );
		frame.duration_sec = static_cast<float>( std::max( duration_ms, 1 ) ) / 1000.f;
		clip->mFrames.push_back( frame );
	}

	if( clip->mFrames.empty() )
	{
		LOG_ERROR( "AnimationClip: no frames loaded from " + clip_json_path );
	}

	return clip;
}

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

uint32_t
AnimationClip::GetFrameTexture( int index ) const
{
	return GetFrame( index ).texture_index;
}

glm::ivec2
AnimationClip::GetFrameSize() const
{
	return { mFrameWidth, mFrameHeight };
}
