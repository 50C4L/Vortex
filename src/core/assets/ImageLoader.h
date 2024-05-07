#ifndef _EAGE_IMAGE_LOADER_H
#define _EAGE_IMAGE_LOADER_H

#include <vector>
#include <string>

namespace assets
{
	class ImageLoader
	{
	public:
		struct Image
		{
			std::vector<unsigned char> data;
			int width;
			int height;
			int num_channels;
		};

		ImageLoader();
		virtual ~ImageLoader();

		Image LoadImage( const std::string& filename );
	};
}

#endif // _EAGE_IMAGE_LOADER_H