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
	/// The JSON references a TextureAtlas and lists frames by name with per-frame durations:
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
	/// The entity's sprite mesh must have been attached with unit UVs (uv_min={0,0}, uv_max={1,1})
	/// so that AnimatedSprite can drive the visible region entirely through uv_rect.
	///
	class AnimationClip
	{
	public:
		struct Frame
		{
			glm::vec2 uv_min;
			glm::vec2 uv_max;
			float duration_sec;
		};

		AnimationClip( const std::string& clip_json_path );
		~AnimationClip();

		const Frame& GetFrame( int index ) const;
		int GetFrameCount() const;

	private:
		Frame mDefaultFrame;
		std::vector<Frame> mFrames;
	};
}

#endif // _EAGE_ANIMATION_CLIP_H_
