#ifndef _EAGE_SYSTEMS_ANIMATION_SYSTEM_H_
#define _EAGE_SYSTEMS_ANIMATION_SYSTEM_H_

#include <ecs/ECS.h>
#include <ecs/ResourceStore.h>
#include <ecs/components/Animation.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace assets
{
	class AnimationClip;
}

namespace eage::ecs
{
	class ECSRegistry;
	class RenderSystem;

	///
	/// AnimationSystem: Loads and caches AnimationClip assets; drives AnimatedSpriteComponent playback.
	///
	class AnimationSystem
	{
	public:
		AnimationSystem( ECSRegistry& registry );
		~AnimationSystem();

		void Update( float delta_time );

		ResourceId LoadClip( RenderSystem& render_system, const std::string& clip_json_path );
		const assets::AnimationClip* GetClip( ResourceId clip_id ) const;

		void Attach( Entity entity, ResourceId clip_id );
		void Play( Entity entity, int start_frame = 0 );
		void PlayOnce( Entity entity, int start_frame = 0 );
		void Pause( Entity entity );
		void ShowFrame( Entity entity, int index );
		void SetLoop( Entity entity, bool loop );

		bool IsPlaying( Entity entity ) const;
		bool IsFinished( Entity entity ) const;
		bool HasAnimation( Entity entity ) const;

	private:
		void ShowFrame( AnimatedSpriteComponent& anim, Entity entity );
		AnimatedSpriteComponent* GetAnimComponent( Entity entity );
		const AnimatedSpriteComponent* GetAnimComponent( Entity entity ) const;

		ECSRegistry& mRegistry;
		ResourceStore<std::shared_ptr<const assets::AnimationClip>> mClipStore;
		std::unordered_map<std::string, ResourceId> mClipPathToId;
	};
}

#endif // _EAGE_SYSTEMS_ANIMATION_SYSTEM_H_
