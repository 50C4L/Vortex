#ifndef _EAGE_ANIMATION_LOAD_DELEGATE_H_
#define _EAGE_ANIMATION_LOAD_DELEGATE_H_

#include <assets/SceneResourceLoader.h>

namespace eage::ecs
{
	class AnimationSystem;
	class RenderSystem;

	class AnimationLoadDelegate : public assets::SceneResourceLoader::Delegate
	{
	public:
		static constexpr const char* SECTION_KEY = assets::SceneResourceLoader::SECTION_ANIMATIONS;

		AnimationLoadDelegate( AnimationSystem& animation_system, RenderSystem& render_system );

		bool Load( const rapidjson::Value& section,
				   assets::SceneResourceLoader::ResourceTable& out ) override;

	private:
		AnimationSystem& mAnimationSystem;
		RenderSystem& mRenderSystem;
	};
}

#endif // _EAGE_ANIMATION_LOAD_DELEGATE_H_
