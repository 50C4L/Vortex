#include "AnimationLoadDelegate.h"

#include <ecs/systems/AnimationSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace utility;

AnimationLoadDelegate::AnimationLoadDelegate( AnimationSystem& animation_system,
											  RenderSystem& render_system )
	: mAnimationSystem( animation_system )
	, mRenderSystem( render_system )
{
}

bool
AnimationLoadDelegate::Load( const rapidjson::Value& section,
							 assets::SceneResourceLoader::ResourceTable& out )
{
	if( !section.IsArray() )
	{
		LOG_ERROR() << "AnimationLoadDelegate: section must be an array";
		return false;
	}

	bool ok = true;
	for( const auto& animation_val : section.GetArray() )
	{
		if( !animation_val.IsString() )
		{
			LOG_ERROR() << "AnimationLoadDelegate: animation entry must be a string path";
			ok = false;
			continue;
		}

		const std::string path = animation_val.GetString();
		const ResourceId clip_id = mAnimationSystem.LoadClip( mRenderSystem, path );
		if( clip_id == INVALID_ID )
		{
			LOG_ERROR() << "AnimationLoadDelegate: failed to load animation clip " << path;
			ok = false;
			continue;
		}

		out[path] = clip_id;
	}

	return ok;
}
