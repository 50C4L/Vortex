#ifndef _ANIMTOOL_FRAME_SEQUENCE_H_
#define _ANIMTOOL_FRAME_SEQUENCE_H_

#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

#include <assets/ImageLoader.h>

namespace eage::graphics
{
	class Renderer;
}

namespace animtool
{

class FrameThumbnail;

class FrameSequence
{
public:
	size_t GetFrameCount() const;
	const FrameThumbnail& GetFrame( size_t index ) const;

	std::optional<size_t> GetSelectedFrame() const;
	void SetSelectedFrame( std::optional<size_t> index );

	size_t AppendPngFolder( const std::filesystem::path& folder, eage::graphics::Renderer& renderer );
	size_t AppendGif( const std::filesystem::path& gif_path, eage::graphics::Renderer& renderer );

	void Clear();

private:
	size_t AppendFrame(
		eage::graphics::Renderer& renderer,
		const std::filesystem::path& source_path,
		const assets::ImageLoader::Image& image,
		int frame_index_in_source,
		int delay_ms );

	bool ContainsFrame( const std::filesystem::path& path, int frame_index_in_source ) const;

	std::vector<FrameThumbnail> mFrames;
	std::optional<size_t> mSelectedFrame;
};

}

#endif // _ANIMTOOL_FRAME_SEQUENCE_H_
