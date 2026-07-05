#ifndef _ANIMTOOL_ANIMATION_EXPORTER_H_
#define _ANIMTOOL_ANIMATION_EXPORTER_H_

#include <filesystem>

namespace animtool
{

class FrameSequence;

class AnimationExporter
{
public:
	static bool Export( const FrameSequence& sequence, const std::filesystem::path& output_directory );
};

}

#endif // _ANIMTOOL_ANIMATION_EXPORTER_H_
