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
	/// Atlas-based clips update UV rects on the entity RenderComponent.
	/// Per-frame texture clips (AnimTool export) require a RenderSystem and material id
	/// so each frame can swap the bound texture.
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
