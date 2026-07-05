#ifndef _ANIMTOOL_FRAME_SEQUENCE_H_
#define _ANIMTOOL_FRAME_SEQUENCE_H_

#include <cstddef>
#include <filesystem>
#include <vector>

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

	size_t AppendPngFolder( const std::filesystem::path& folder, eage::graphics::Renderer& renderer );

	void Clear();

private:
	bool ContainsPath( const std::filesystem::path& path ) const;

	std::vector<FrameThumbnail> mFrames;
};

}

#endif // _ANIMTOOL_FRAME_SEQUENCE_H_
