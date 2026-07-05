#ifndef _EAGE_ANIMATION_CLIP_H_
#define _EAGE_ANIMATION_CLIP_H_

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace assets
{
	///
	/// AnimationClip: loads a frame-sequence animation from a JSON file.
	///
	/// Atlas-based format (packed texture):
	///
	///   {
	///     "atlas": "./resources/textures/ship/ship_texatlas.json",
	///     "flip": true,
	///     "frames": [
	///       { "name": "walk_0.png", "duration_ms": 100 },
	///       { "name": "walk_1.png", "duration_ms": 100 }
	///     ]
	///   }
	///
	/// Per-frame texture format (AnimTool export):
	///
	///   {
	///     "flip": false,
	///     "frames": [
	///       { "texture": "frame_000.png", "duration_ms": 100 },
	///       { "texture": "frame_001.png", "duration_ms": 100 }
	///     ]
	///   }
	///
	/// Texture paths are relative to the animation JSON file directory.
	///
	class AnimationClip
	{
	public:
		struct Frame
		{
			glm::vec2 uv_min;
			glm::vec2 uv_max;
			float duration_sec;
			std::string texture_path;
		};

		AnimationClip( const std::string& clip_json_path );
		~AnimationClip();

		const Frame& GetFrame( int index ) const;
		int GetFrameCount() const;

		bool UsesPerFrameTextures() const;
		const std::string& GetClipDirectory() const;
		std::string GetResolvedTexturePath( int index ) const;

	private:
		Frame mDefaultFrame;
		std::vector<Frame> mFrames;
		std::string mClipDirectory;
		bool mUsesPerFrameTextures = false;
	};
}

#endif // _EAGE_ANIMATION_CLIP_H_
