#ifndef _EAGE_ANIMATED_SPRITE_H_
#define _EAGE_ANIMATED_SPRITE_H_

#include <assets/AnimationClip.h>
#include <ecs/ECS.h>

namespace eage::animation
{
	///
	/// AnimatedSprite: per-entity playback controller for an AnimationClip.
	///
	/// The AnimationClip must outlive this AnimatedSprite.
	///
	/// Usage:
	///   auto clip = assets::AnimationClip::Load( render_system, "path/to/animation.json" );
	///   eage::animation::AnimatedSprite anim( *clip, entity, registry );
	///   anim.ShowFrame( 0 );
	///   anim.Pause();
	///   anim.PlayOnce( 0 );
	///   anim.Update( delta_time_sec );
	///
	class AnimatedSprite
	{
	public:
		AnimatedSprite( const assets::AnimationClip& clip, eage::ecs::Entity entity, eage::ecs::ECSRegistry& registry );
		~AnimatedSprite();

		void Play( int start_frame = 0 );
		void PlayOnce( int start_frame = 0 );
		void Pause();
		void ShowFrame( int index );
		void SetLoop( bool loop );

		bool IsPlaying() const;
		bool IsFinished() const;

		void Update( float delta_time_sec );

	private:
		const assets::AnimationClip& mClip;
		eage::ecs::Entity mEntity;
		eage::ecs::ECSRegistry& mRegistry;

		int mCurrentFrame = 0;
		float mElapsed = 0.f;
		bool mPlaying = false;
		bool mLoop = true;
		bool mFinished = false;
	};
}

#endif // _EAGE_ANIMATED_SPRITE_H_
