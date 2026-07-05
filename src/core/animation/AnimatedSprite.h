#ifndef _EAGE_ANIMATED_SPRITE_H_
#define _EAGE_ANIMATED_SPRITE_H_

#include <assets/AnimationClip.h>
#include <ecs/ECS.h>
#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	class RenderSystem;
}

namespace eage::animation
{
	///
	/// AnimatedSprite: drives frame-based sprite animation for a single entity.
	///
	/// Supports two AnimationClip formats:
	///
	///   1. Atlas-based — frames reference subtextures in a packed atlas. UV rects on the
	///      entity RenderComponent are updated each frame. Attach the sprite with unit UVs
	///      (uv_min={0,0}, uv_max={1,1}) so uv_rect controls the visible region.
	///
	///   2. Per-frame textures — frames reference individual PNG files (AnimTool export).
	///      Requires RenderSystem and a material id; textures are preloaded and swapped
	///      each frame. Attach the sprite with unit UVs (full texture visible each frame).
	///
	/// Usage (atlas clip):
	///   assets::AnimationClip clip( "resources/textures/ship/ship_anim.json" );
	///   eage::animation::AnimatedSprite anim( clip, entity, registry );
	///
	/// Usage (per-frame texture clip):
	///   assets::AnimationClip clip( "resources/animations/walk/animation.json" );
	///   eage::animation::AnimatedSprite anim( clip, entity, registry, &render_system, material_id );
	///
	///   anim.SetLoop( true );
	///   anim.Play();
	///   anim.Update( delta_time_sec );
	///
	class AnimatedSprite
	{
	public:
		AnimatedSprite(
			const assets::AnimationClip& clip,
			eage::ecs::Entity entity,
			eage::ecs::ECSRegistry& registry,
			eage::ecs::RenderSystem* render_system = nullptr,
			eage::ecs::ResourceId material_id = eage::ecs::INVALID_ID );
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
		eage::ecs::RenderSystem* mRenderSystem = nullptr;
		eage::ecs::ResourceId mMaterialId = eage::ecs::INVALID_ID;

		int mCurrentFrame = 0;
		float mElapsed = 0.f;
		bool mPlaying = false;
		bool mLoop = true;
	};
}

#endif // _EAGE_ANIMATED_SPRITE_H_
