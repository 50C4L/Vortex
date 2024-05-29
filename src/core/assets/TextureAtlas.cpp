#include "TextureAtlas.h"

#include <utility/Logger.h>
#include <utility/Filesystem.h>
#include <utility/JsonParser.h>

using namespace utility;
using namespace assets;

TextureAtlas::TextureAtlas( const std::string& atlas_json_path )
{
	LOG( "Loading texture atlas: " + atlas_json_path );

	mDefaultSubTexture.name = "default";
	mDefaultSubTexture.uv_min = glm::vec2( 0.0f, 0.0f );
	mDefaultSubTexture.uv_max = glm::vec2( 1.0f, 1.0f );

	if( !is_file_exist( atlas_json_path ) )
	{
		LOG_ERROR( "Failed to load texture atlas, file doesn't exist: " + atlas_json_path );
	}

	rapidjson::Document document;
	if( parse_json_document( document, atlas_json_path ) )
	{
		const auto& meta = get_json_object( document, "meta" );
		const auto& size = get_json_object( meta, "size" );
		const int width = get_json_int( size, "w" );
		const int height = get_json_int( size, "h" );

		const auto& frames = get_json_array( document, "frames" );
		for( const auto& frame : frames.GetArray() )
		{
			const std::string name = get_json_string( frame, "filename" );
			const auto& frame_data = get_json_object( frame, "frame" );
			const int x = get_json_int( frame_data, "x" );
			const int y = get_json_int( frame_data, "y" );
			const int w = get_json_int( frame_data, "w" );
			const int h = get_json_int( frame_data, "h" );

			SubTexture sub_texture;
			sub_texture.name = name;
			sub_texture.uv_min = glm::vec2( static_cast<float>( x ) / width, static_cast<float>( y ) / height );
			sub_texture.uv_max = glm::vec2( static_cast<float>( x + w ) / width, static_cast<float>( y + h ) / height );

			mSubTextures[ name ] = sub_texture;
		}
	}
}

TextureAtlas::~TextureAtlas()
{
}

const TextureAtlas::SubTexture&
TextureAtlas::GetSubTexture( const std::string& name ) const
{
	auto it = mSubTextures.find( name );
	if( it == mSubTextures.end() )
	{
		LOG_ERROR( "SubTexture not found: " + name );
		return mDefaultSubTexture;
	}
	return it->second;
}

void
TextureAtlas::Flip()
{
	for( auto& sub_texture : mSubTextures )
	{
		float temp_y = sub_texture.second.uv_min.y;
		sub_texture.second.uv_min.y = 1.0f - sub_texture.second.uv_max.y;
		sub_texture.second.uv_max.y = 1.0f - temp_y;
	}
}

void
TextureAtlas::Flip( const std::string& name )
{
	auto it = mSubTextures.find( name );
	if( it == mSubTextures.end() )
	{
		LOG_ERROR( "SubTexture not found: " + name );
		return;
	}
	float temp_y = it->second.uv_min.y;
	it->second.uv_min.y = 1.0f - it->second.uv_max.y;
	it->second.uv_max.y = 1.0f - temp_y;
}
