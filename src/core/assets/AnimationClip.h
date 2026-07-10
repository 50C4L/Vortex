#ifndef _EAGE_ANIMATION_CLIP_H_
#define _EAGE_ANIMATION_CLIP_H_

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace eage::ecs
{
	class RenderSystem;
}

namespace assets
{
	///
	/// AnimationClip: bindless frame-sequence animation loaded from an animtool export.
	///
	///   {
	///     "flip": false,
	///     "frames": [
	///       { "texture": "frame_000.png", "duration_ms": 50 },
	///       { "texture": "frame_001.png", "duration_ms": 50 }
	///     ]
	///   }
	///
	/// Each frame PNG is registered via RenderSystem::CreateTexture(). AnimatedSprite
	/// drives RenderComponent::texture_index per frame at playback time.
	///
	class AnimationClip
	{
	public:
		struct Frame
		{
			uint32_t texture_index = 0;
			float duration_sec = 0.1f;
		};

		static std::shared_ptr<AnimationClip> Load(
			eage::ecs::RenderSystem& render_system,
			const std::string& clip_json_path );

		const Frame& GetFrame( int index ) const;
		int GetFrameCount() const;
		uint32_t GetFrameTexture( int index ) const;
		glm::ivec2 GetFrameSize() const;

	private:
		AnimationClip() = default;

		Frame mDefaultFrame;
		std::vector<Frame> mFrames;
		int mFrameWidth = 0;
		int mFrameHeight = 0;
	};
}

#endif // _EAGE_ANIMATION_CLIP_H_
