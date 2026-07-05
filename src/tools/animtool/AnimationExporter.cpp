#include "AnimationExporter.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include <assets/ImageLoader.h>
#include <assets/ImageWriter.h>
#include <utility/Logger.h>

#include "FrameSequence.h"
#include "FrameThumbnail.h"

using namespace animtool;

namespace
{
	std::string format_frame_filename( size_t frame_index )
	{
		std::ostringstream stream;
		stream << "frame_" << std::setw( 3 ) << std::setfill( '0' ) << frame_index << ".png";
		return stream.str();
	}

	bool is_gif_path( const std::filesystem::path& path )
	{
		const std::string extension = path.extension().string();
		if( extension.size() != 4 )
		{
			return false;
		}

		return std::tolower( static_cast<unsigned char>( extension[1] ) ) == 'g'
			&& std::tolower( static_cast<unsigned char>( extension[2] ) ) == 'i'
			&& std::tolower( static_cast<unsigned char>( extension[3] ) ) == 'f';
	}

	void flip_image_vertically( assets::ImageLoader::Image& image )
	{
		const int stride = image.width * image.num_channels;
		std::vector<unsigned char> row( static_cast<size_t>( stride ) );

		for( int y = 0; y < image.height / 2; ++y )
		{
			unsigned char* top_row = image.data.data() + static_cast<size_t>( y ) * static_cast<size_t>( stride );
			unsigned char* bottom_row = image.data.data() + static_cast<size_t>( image.height - 1 - y ) * static_cast<size_t>( stride );
			std::memcpy( row.data(), top_row, static_cast<size_t>( stride ) );
			std::memcpy( top_row, bottom_row, static_cast<size_t>( stride ) );
			std::memcpy( bottom_row, row.data(), static_cast<size_t>( stride ) );
		}
	}

	bool export_png_copy( const std::filesystem::path& source_path, const std::filesystem::path& destination_path )
	{
		std::error_code error_code;
		std::filesystem::copy_file(
			source_path,
			destination_path,
			std::filesystem::copy_options::overwrite_existing,
			error_code );

		if( error_code )
		{
			utility::LOG_ERROR() << "Failed to copy PNG: " << source_path.string() << " -> " << destination_path.string();
			return false;
		}

		return true;
	}

	bool export_gif_frame(
		const std::filesystem::path& source_path,
		int frame_index_in_source,
		const std::filesystem::path& destination_path )
	{
		assets::ImageLoader image_loader;
		const std::vector<assets::ImageLoader::GifFrame> gif_frames = image_loader.LoadGifFrames( source_path.string() );
		if( frame_index_in_source < 0 || frame_index_in_source >= static_cast<int>( gif_frames.size() ) )
		{
			utility::LOG_ERROR() << "GIF frame index out of range: " << frame_index_in_source;
			return false;
		}

		assets::ImageLoader::Image image = gif_frames[static_cast<size_t>( frame_index_in_source )].image;
		flip_image_vertically( image );
		return assets::ImageWriter::WritePng( destination_path.string(), image );
	}

	bool write_animation_json(
		const std::filesystem::path& output_directory,
		const std::vector<std::pair<std::string, int>>& exported_frames )
	{
		rapidjson::Document document;
		document.SetObject();
		auto& allocator = document.GetAllocator();

		document.AddMember( "flip", false, allocator );

		rapidjson::Value frames( rapidjson::kArrayType );
		for( const auto& [texture_name, duration_ms] : exported_frames )
		{
			rapidjson::Value frame_object( rapidjson::kObjectType );
			frame_object.AddMember(
				"texture",
				rapidjson::Value( texture_name.c_str(), allocator ),
				allocator );
			frame_object.AddMember( "duration_ms", duration_ms, allocator );
			frames.PushBack( frame_object, allocator );
		}

		document.AddMember( "frames", frames, allocator );

		const std::filesystem::path json_path = output_directory / "animation.json";
		std::ofstream output_stream( json_path );
		if( !output_stream )
		{
			utility::LOG_ERROR() << "Failed to open animation JSON for writing: " << json_path.string();
			return false;
		}

		rapidjson::OStreamWrapper stream_wrapper( output_stream );
		rapidjson::Writer<rapidjson::OStreamWrapper> writer( stream_wrapper );
		document.Accept( writer );
		return true;
	}
}

bool
AnimationExporter::Export( const FrameSequence& sequence, const std::filesystem::path& output_directory )
{
	if( sequence.GetFrameCount() == 0 )
	{
		utility::LOG_ERROR() << "Cannot export an empty frame sequence.";
		return false;
	}

	std::error_code error_code;
	std::filesystem::create_directories( output_directory, error_code );
	if( error_code )
	{
		utility::LOG_ERROR() << "Failed to create export directory: " << output_directory.string();
		return false;
	}

	std::vector<std::pair<std::string, int>> exported_frames;
	exported_frames.reserve( sequence.GetFrameCount() );

	for( size_t frame_index = 0; frame_index < sequence.GetFrameCount(); ++frame_index )
	{
		const FrameThumbnail& frame = sequence.GetFrame( frame_index );
		const std::string texture_name = format_frame_filename( frame_index );
		const std::filesystem::path destination_path = output_directory / texture_name;

		bool exported = false;
		if( is_gif_path( frame.GetSourcePath() ) )
		{
			exported = export_gif_frame( frame.GetSourcePath(), frame.GetFrameIndexInSource(), destination_path );
		}
		else
		{
			exported = export_png_copy( frame.GetSourcePath(), destination_path );
		}

		if( !exported )
		{
			utility::LOG_ERROR() << "Failed to export frame " << frame_index;
			return false;
		}

		exported_frames.emplace_back( texture_name, frame.GetDelayMs() );
	}

	if( !write_animation_json( output_directory, exported_frames ) )
	{
		return false;
	}

	utility::LOG() << "Exported " << exported_frames.size() << " frame(s) to: " << output_directory.string();
	return true;
}
