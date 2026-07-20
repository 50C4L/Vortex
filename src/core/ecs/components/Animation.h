#ifndef _EAGE_COMPONENTS_ANIMATION_H_
#define _EAGE_COMPONENTS_ANIMATION_H_

#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	struct AnimatedSpriteComponent
	{
		ResourceId clip_id = INVALID_ID;
		int current_frame = 0;
		float elapsed = 0.f;
		bool playing = false;
		bool loop = true;
		bool finished = false;
	};
}

#endif // _EAGE_COMPONENTS_ANIMATION_H_
