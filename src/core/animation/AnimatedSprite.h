#ifndef _EAGE_ANIMATED_SPRITE_H_
#define _EAGE_ANIMATED_SPRITE_H_

#include <assets/AnimationClip.h>
#include <ecs/ECS.h>

namespace eage::animation
{
	///
	/// AnimatedSprite: drives frame-based sprite animation for a single entity.
	///
	/// Requires the entity to have a RenderComponent attached with unit UVs
	/// (uv_min={0,0}, uv_max={1,1}) so that uv_rect fully controls the visible region.
	///
	/// Usage:
	///   assets::AnimationClip clip( "path/to/clip.json" );
	///   eage::animation::AnimatedSprite anim( clip, entity, registry );
	///   anim.SetLoop( true );
	///   anim.Play();
	///   // each frame:
	///   anim.Update( delta_time_sec );
	///
	class AnimatedSprite
	{
	public:
		AnimatedSprite( const assets::AnimationClip& clip, eage::ecs::Entity entity, eage::ecs::ECSRegistry& registry );
		~AnimatedSprite();

		void Play();
		void Pause();
		void ShowFrame( int index );
		void SetLoop( bool loop );

		void Update( float delta_time_sec );

	private:
		const assets::AnimationClip& mClip;
		eage::ecs::Entity mEntity;
		eage::ecs::ECSRegistry& mRegistry;

		int mCurrentFrame = 0;
		float mElapsed = 0.f;
		bool mPlaying = false;
		bool mLoop = true;
	};
}

#endif // _EAGE_ANIMATED_SPRITE_H_
