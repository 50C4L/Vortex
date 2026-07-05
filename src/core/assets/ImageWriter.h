#ifndef _EAGE_IMAGE_WRITER_H_
#define _EAGE_IMAGE_WRITER_H_

#include <string>

#include "ImageLoader.h"

namespace assets
{
	class ImageWriter
	{
	public:
		static bool WritePng( const std::string& filename, const ImageLoader::Image& image );
	};
}

#endif // _EAGE_IMAGE_WRITER_H_
