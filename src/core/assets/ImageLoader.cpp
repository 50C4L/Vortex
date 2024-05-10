#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace assets;

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
	unsigned char* data = stbi_load( filename.c_str(), &image.width, &image.height, &image.num_channels, 0 );
	if( data )
	{
		image.data = std::vector<unsigned char>( data, data + image.width * image.height * image.num_channels );
		stbi_image_free( data );
	}

	return image;
}