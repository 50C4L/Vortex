#ifndef _EAGE_COMPONENTS_EFFECT_H_
#define _EAGE_COMPONENTS_EFFECT_H_

#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	///
	/// Marks a pooled FX entity managed by EffectSystem.
	/// effect_id identifies the EffectDefinition (not an AnimationClip).
	///
	struct EffectInstanceComponent
	{
		ResourceId effect_id = INVALID_ID;
		bool active = false;
	};
}

#endif // _EAGE_COMPONENTS_EFFECT_H_
