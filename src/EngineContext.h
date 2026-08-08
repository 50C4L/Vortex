#ifndef _VORTEX_ENGINE_CONTEXT_H_
#define _VORTEX_ENGINE_CONTEXT_H_

namespace eage::graphics
{
	class Renderer;
}

namespace eage::ui
{
	class UISystem;
}

namespace eage::ecs
{
	class AnimationSystem;
	class AudioSystem;
	class ECSRegistry;
	class EffectSystem;
	class PhysicsSystem;
	class RenderSystem;
	class SceneGraphSystem;
}

namespace assets
{
	class SceneResourceLoader;
}

namespace events
{
	class InputController;
}

namespace vortex
{
	struct EngineContext
	{
		eage::graphics::Renderer& renderer;
		eage::ui::UISystem&       ui_system;
		eage::ecs::ECSRegistry&   registry;
		eage::ecs::RenderSystem&  render_system;
		eage::ecs::PhysicsSystem& physics_system;
		eage::ecs::AudioSystem&   audio_system;
		eage::ecs::AnimationSystem& animation_system;
		eage::ecs::EffectSystem&  effect_system;
		eage::ecs::SceneGraphSystem& scene_graph_system;
		assets::SceneResourceLoader& resource_loader;
		events::InputController&  input;
	};
}

#endif // _VORTEX_ENGINE_CONTEXT_H_
