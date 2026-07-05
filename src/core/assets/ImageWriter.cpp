#include "ImageWriter.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <utility/Logger.h>

using namespace assets;
using namespace utility;

bool
ImageWriter::WritePng( const std::string& filename, const ImageLoader::Image& image )
{
	if( image.data.empty() || image.width <= 0 || image.height <= 0 )
	{
		LOG_ERROR( "Cannot write invalid image: " + filename );
		return false;
	}

	const int stride = image.width * image.num_channels;
	const int result = stbi_write_png(
		filename.c_str(),
		image.width,
		image.height,
		image.num_channels,
		image.data.data(),
		stride );

	if( result == 0 )
	{
		LOG_ERROR( "Failed to write PNG: " + filename );
		return false;
	}

	return true;
}
