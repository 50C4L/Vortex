#include "FrameSequence.h"

#include <algorithm>
#include <cctype>

#include <assets/ImageLoader.h>
#include <utility/Logger.h>

#include "FrameThumbnail.h"

using namespace animtool;

namespace
{
	constexpr int DEFAULT_ANIMATION_DURATION_MS = 100;

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

int
FrameSequence::GetAnimationDurationMs() const
{
	return mAnimationDurationMs;
}

int&
FrameSequence::GetAnimationDurationMs()
{
	return mAnimationDurationMs;
}

void
FrameSequence::SetAnimationDurationMs( int duration_ms )
{
	mAnimationDurationMs = std::max( duration_ms, 1 );
}

float
FrameSequence::GetFrameDurationSec() const
{
	if( mFrames.empty() )
	{
		return 0.f;
	}

	return static_cast<float>( mAnimationDurationMs )
		/ static_cast<float>( mFrames.size() )
		/ 1000.f;
}

int
FrameSequence::GetFrameDurationMs( size_t frame_index ) const
{
	const size_t frame_count = mFrames.size();
	if( frame_count == 0 )
	{
		return 1;
	}

	const int base_duration_ms = mAnimationDurationMs / static_cast<int>( frame_count );
	const int remainder_ms = mAnimationDurationMs % static_cast<int>( frame_count );
	const int extra_ms = frame_index < static_cast<size_t>( remainder_ms ) ? 1 : 0;
	return std::max( base_duration_ms + extra_ms, 1 );
}

std::optional<size_t>
FrameSequence::GetSelectedFrame() const
{
	return mSelectedFrame;
}

void
FrameSequence::SetSelectedFrame( std::optional<size_t> index )
{
	if( index.has_value() && index.value() >= mFrames.size() )
	{
		mSelectedFrame = std::nullopt;
		return;
	}

	mSelectedFrame = index;
}

size_t
FrameSequence::AppendFrame(
	eage::graphics::Renderer& renderer,
	const std::filesystem::path& source_path,
	const assets::ImageLoader::Image& image,
	int frame_index_in_source,
	int source_delay_ms )
{
	if( ContainsFrame( source_path, frame_index_in_source ) )
	{
		return 0;
	}

	if( image.data.empty() || image.width <= 0 || image.height <= 0 )
	{
		return 0;
	}

	FrameThumbnail frame;
	frame.Upload( renderer, source_path, image, frame_index_in_source, source_delay_ms );
	mFrames.push_back( std::move( frame ) );

	if( !mSelectedFrame.has_value() )
	{
		mSelectedFrame = mFrames.size() - 1u;
	}

	return 1;
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

	const bool was_empty = mFrames.empty();

	assets::ImageLoader image_loader;
	size_t added_count = 0;

	for( const std::filesystem::path& png_path : png_files )
	{
		const std::filesystem::path absolute_path = canonical_path( png_path );
		const assets::ImageLoader::Image image = image_loader.LoadImage( png_path.string() );
		if( image.data.empty() || image.width <= 0 || image.height <= 0 )
		{
			utility::LOG_ERROR() << "Skipping invalid image: " << png_path.string();
			continue;
		}

		added_count += AppendFrame( renderer, absolute_path, image, 0, DEFAULT_ANIMATION_DURATION_MS );
	}

	if( was_empty && added_count > 0 )
	{
		mAnimationDurationMs = DEFAULT_ANIMATION_DURATION_MS;
	}

	return added_count;
}

size_t
FrameSequence::AppendGif( const std::filesystem::path& gif_path, eage::graphics::Renderer& renderer )
{
	const std::filesystem::path absolute_path = canonical_path( gif_path );

	assets::ImageLoader image_loader;
	const std::vector<assets::ImageLoader::GifFrame> gif_frames = image_loader.LoadGifFrames( gif_path.string() );
	if( gif_frames.empty() )
	{
		utility::LOG_ERROR() << "No frames loaded from GIF: " << gif_path.string();
		return 0;
	}

	const bool was_empty = mFrames.empty();
	int gif_total_duration_ms = 0;

	size_t added_count = 0;
	for( size_t frame_index = 0; frame_index < gif_frames.size(); ++frame_index )
	{
		const assets::ImageLoader::GifFrame& gif_frame = gif_frames[frame_index];
		gif_total_duration_ms += std::max( gif_frame.delay_ms, 1 );
		added_count += AppendFrame(
			renderer,
			absolute_path,
			gif_frame.image,
			static_cast<int>( frame_index ),
			gif_frame.delay_ms );
	}

	if( was_empty && added_count > 0 )
	{
		mAnimationDurationMs = std::max( gif_total_duration_ms, 1 );
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
	mSelectedFrame = std::nullopt;
	mAnimationDurationMs = DEFAULT_ANIMATION_DURATION_MS;
}

bool
FrameSequence::ContainsFrame( const std::filesystem::path& path, int frame_index_in_source ) const
{
	for( const FrameThumbnail& frame : mFrames )
	{
		if( frame.GetSourcePath() == path && frame.GetFrameIndexInSource() == frame_index_in_source )
		{
			return true;
		}
	}

	return false;
}
