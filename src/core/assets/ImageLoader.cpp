#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <fstream>
#include <utility/Logger.h>

using namespace assets;
using namespace utility;

ImageLoader::ImageLoader()
{
	stbi_set_flip_vertically_on_load( true );
}

ImageLoader::~ImageLoader()
{
}

ImageLoader::Image
ImageLoader::LoadImage( const std::string& filename )
{
	Image image;
	int original_channels = 0;
	unsigned char* data = stbi_load( filename.c_str(), &image.width, &image.height, &original_channels, 4 );
	image.num_channels = 4;
	if( data )
	{
		image.data = std::vector<unsigned char>( data, data + image.width * image.height * image.num_channels );
		stbi_image_free( data );
	}
	else
	{
		LOG_ERROR( "Failed to load image: " + filename );
	}

	return image;
}

std::vector<ImageLoader::GifFrame>
ImageLoader::LoadGifFrames( const std::string& filename )
{
	std::vector<GifFrame> gif_frames;

	std::ifstream file( filename, std::ios::binary | std::ios::ate );
	if( !file )
	{
		LOG_ERROR( "Failed to open GIF: " + filename );
		return gif_frames;
	}

	const std::streamsize file_size = file.tellg();
	if( file_size <= 0 )
	{
		LOG_ERROR( "GIF file is empty: " + filename );
		return gif_frames;
	}

	file.seekg( 0, std::ios::beg );
	std::vector<unsigned char> file_data( static_cast<size_t>( file_size ) );
	if( !file.read( reinterpret_cast<char*>( file_data.data() ), file_size ) )
	{
		LOG_ERROR( "Failed to read GIF: " + filename );
		return gif_frames;
	}

	int* delays = nullptr;
	int width = 0;
	int height = 0;
	int frame_count = 0;
	int channels = 0;

	unsigned char* data = stbi_load_gif_from_memory(
		file_data.data(),
		static_cast<int>( file_data.size() ),
		&delays,
		&width,
		&height,
		&frame_count,
		&channels,
		4 );

	if( !data || width <= 0 || height <= 0 || frame_count <= 0 )
	{
		LOG_ERROR( "Failed to load GIF: " + filename );
		if( data )
		{
			stbi_image_free( data );
		}
		if( delays )
		{
			STBI_FREE( delays );
		}
		return gif_frames;
	}

	const size_t frame_byte_size = static_cast<size_t>( width ) * static_cast<size_t>( height ) * 4u;
	gif_frames.reserve( static_cast<size_t>( frame_count ) );

	for( int frame_index = 0; frame_index < frame_count; ++frame_index )
	{
		GifFrame gif_frame;
		gif_frame.image.width = width;
		gif_frame.image.height = height;
		gif_frame.image.num_channels = 4;
		gif_frame.image.data.assign(
			data + frame_byte_size * static_cast<size_t>( frame_index ),
			data + frame_byte_size * static_cast<size_t>( frame_index + 1 ) );
		gif_frame.delay_ms = delays ? delays[frame_index] : 100;
		if( gif_frame.delay_ms <= 0 )
		{
			gif_frame.delay_ms = 100;
		}

		gif_frames.push_back( std::move( gif_frame ) );
	}

	stbi_image_free( data );
	if( delays )
	{
		STBI_FREE( delays );
	}

	return gif_frames;
}