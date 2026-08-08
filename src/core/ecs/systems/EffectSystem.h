#ifndef _EAGE_SYSTEMS_EFFECT_SYSTEM_H_
#define _EAGE_SYSTEMS_EFFECT_SYSTEM_H_

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <ecs/ECS.h>
#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	class AnimationSystem;
	class RenderSystem;
	class SceneGraphSystem;

	///
	/// EffectSystem: Pools fire-and-forget visual FX entities.
	///
	/// ResourceId returned by Create() identifies an effect definition (clip list +
	/// entity pool), not an AnimationClip. Future effects may hold multiple clips;
	/// Apply() currently plays clip_ids[0].
	///
	class EffectSystem
	{
	public:
		struct EffectConfig
		{
			std::vector<ResourceId> clip_ids; // non-empty; [0] = primary
			ResourceId material_id = INVALID_ID;
			ResourceId sound_id = INVALID_ID; // optional; INVALID_ID = silent effect
			int pool_size = 16;
		};

		EffectSystem( ECSRegistry& registry, AnimationSystem& animation_system,
					  SceneGraphSystem& scene_graph_system );
		~EffectSystem();

		/// Pre-create a pool of FX entities. Returns an effect ResourceId.
		ResourceId Create( RenderSystem& render_system, const EffectConfig& config, Entity root_entity );

		/// Play the effect at world position (primary clip). Returns false if pool empty or invalid.
		/// rotation defaults to identity; pass glm::angleAxis( ... ) for a custom orientation.
		bool Apply( ResourceId effect_id, glm::vec2 pos, const glm::quat& rotation = glm::quat() );

		/// Recycle finished active instances. Call after AnimationSystem::Update.
		void Update();

	private:
		struct EffectDefinition
		{
			std::vector<ResourceId> clip_ids;
			ResourceId material_id = INVALID_ID;
			ResourceId mesh_id = INVALID_ID;
			ResourceId sound_id = INVALID_ID;
			std::deque<Entity> available;
			std::unordered_set<Entity> all;
		};

		void Recycle( Entity entity, EffectDefinition& definition );

		ECSRegistry& mRegistry;
		AnimationSystem& mAnimationSystem;
		SceneGraphSystem& mSceneGraphSystem;
		std::unordered_map<ResourceId, EffectDefinition> mEffects;
		ResourceId mNextEffectId = 1;
	};
}

#endif // _EAGE_SYSTEMS_EFFECT_SYSTEM_H_
