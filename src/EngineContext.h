#ifndef _VORTEX_ENGINE_CONTEXT_H_
#define _VORTEX_ENGINE_CONTEXT_H_

namespace eage::ecs
{
	class AnimationSystem;
	class AudioSystem;
	class ECSRegistry;
	class PhysicsSystem;
	class RenderSystem;
}

namespace events
{
	class InputController;
}

namespace vortex
{
	struct EngineContext
	{
		eage::ecs::ECSRegistry&   registry;
		eage::ecs::RenderSystem&  render_system;
		eage::ecs::PhysicsSystem& physics_system;
		eage::ecs::AudioSystem&   audio_system;
		eage::ecs::AnimationSystem& animation_system;
		events::InputController&  input;
	};
}

#endif // _VORTEX_ENGINE_CONTEXT_H_
