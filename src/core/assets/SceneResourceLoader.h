#ifndef _EAGE_SCENE_RESOURCE_LOADER_H_
#define _EAGE_SCENE_RESOURCE_LOADER_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	class AnimationSystem;
	class AudioSystem;
	class RenderSystem;
}

namespace assets
{
	///
	/// SceneResourceLoader: Parses a scene manifest JSON and eagerly loads file-backed assets.
	/// Assets are looked up by their file path (the manifest URI / public ID).
	///
	class SceneResourceLoader
	{
	public:
		SceneResourceLoader( eage::ecs::RenderSystem& render_system,
							 eage::ecs::AnimationSystem& animation_system,
							 eage::ecs::AudioSystem& audio_system );

		bool LoadManifest( const std::string& manifest_path );

		uint32_t GetTexture( const std::string& path ) const;
		eage::ecs::ResourceId GetClip( const std::string& path ) const;
		eage::ecs::ResourceId GetSound( const std::string& path ) const;

	private:
		eage::ecs::RenderSystem& mRenderSystem;
		eage::ecs::AnimationSystem& mAnimationSystem;
		eage::ecs::AudioSystem& mAudioSystem;

		std::unordered_map<std::string, uint32_t> mTextures;
		std::unordered_map<std::string, eage::ecs::ResourceId> mClips;
		std::unordered_map<std::string, eage::ecs::ResourceId> mSounds;
	};
}

#endif // _EAGE_SCENE_RESOURCE_LOADER_H_
