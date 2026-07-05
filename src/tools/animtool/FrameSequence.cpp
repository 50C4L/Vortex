#include "FrameSequence.h"

#include <algorithm>
#include <cctype>

#include <assets/ImageLoader.h>
#include <utility/Logger.h>

#include "FrameThumbnail.h"

using namespace animtool;

namespace
{
	bool is_png_file( const std::filesystem::path& path )
	{
		if( !std::filesystem::is_regular_file( path ) )
		{
			return false;
		}

		const std::string extension = path.extension().string();
		if( extension.size() != 4 )
		{
			return false;
		}

		return std::tolower( static_cast<unsigned char>( extension[1] ) ) == 'p'
			&& std::tolower( static_cast<unsigned char>( extension[2] ) ) == 'n'
			&& std::tolower( static_cast<unsigned char>( extension[3] ) ) == 'g';
	}

	std::vector<std::filesystem::path> collect_png_files( const std::filesystem::path& folder )
	{
		std::vector<std::filesystem::path> png_files;

		if( !std::filesystem::is_directory( folder ) )
		{
			utility::LOG_ERROR() << "Not a directory: " << folder.string();
			return png_files;
		}

		for( const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator( folder ) )
		{
			if( is_png_file( entry.path() ) )
			{
				png_files.push_back( entry.path() );
			}
		}

		std::sort( png_files.begin(), png_files.end(),
			[]( const std::filesystem::path& lhs, const std::filesystem::path& rhs )
			{
				return lhs.filename().string() < rhs.filename().string();
			} );

		return png_files;
	}

	std::filesystem::path canonical_path( const std::filesystem::path& path )
	{
		std::error_code error_code;
		const std::filesystem::path canonical = std::filesystem::weakly_canonical( path, error_code );
		if( error_code )
		{
			return std::filesystem::absolute( path );
		}

		return canonical;
	}
}

size_t
FrameSequence::GetFrameCount() const
{
	return mFrames.size();
}

const FrameThumbnail&
FrameSequence::GetFrame( size_t index ) const
{
	return mFrames.at( index );
}

size_t
FrameSequence::AppendPngFolder( const std::filesystem::path& folder, eage::graphics::Renderer& renderer )
{
	const std::vector<std::filesystem::path> png_files = collect_png_files( folder );
	if( png_files.empty() )
	{
		utility::LOG() << "No PNG files found in folder (top level only): " << folder.string();
		return 0;
	}

	assets::ImageLoader image_loader;
	size_t added_count = 0;

	for( const std::filesystem::path& png_path : png_files )
	{
		const std::filesystem::path absolute_path = canonical_path( png_path );
		if( ContainsPath( absolute_path ) )
		{
			continue;
		}

		const assets::ImageLoader::Image image = image_loader.LoadImage( png_path.string() );
		if( image.data.empty() || image.width <= 0 || image.height <= 0 )
		{
			utility::LOG_ERROR() << "Skipping invalid image: " << png_path.string();
			continue;
		}

		FrameThumbnail frame;
		frame.Upload( renderer, absolute_path, image );
		mFrames.push_back( std::move( frame ) );
		++added_count;
	}

	return added_count;
}

void
FrameSequence::Clear()
{
	for( FrameThumbnail& frame : mFrames )
	{
		frame.ReleaseImGuiTexture();
	}

	mFrames.clear();
}

bool
FrameSequence::ContainsPath( const std::filesystem::path& path ) const
{
	for( const FrameThumbnail& frame : mFrames )
	{
		if( frame.GetSourcePath() == path )
		{
			return true;
		}
	}

	return false;
}
