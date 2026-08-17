#ifndef _VORTEX_ENEMY_SYSTEM_H_
#define _VORTEX_ENEMY_SYSTEM_H_

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <glm/glm.hpp>

#include <ecs/ResourceStore.h>
#include <ecs/systems/PhysicsSystem.h>

#include "../data/EnemyDefinition.h"

namespace assets
{
	class SceneResourceLoader;
}

namespace eage::ecs
{
	class ECSRegistry;
	class EffectSystem;
	class RenderSystem;
	class SceneGraphSystem;
}

namespace vortex
{
	///
	/// EnemySystem: pooled enemies keyed by definition id.
	/// Call PreparePool() per enemy JSON, then Spawn( id, count ).
	///
	class EnemySystem : public eage::ecs::PhysicsSystem::Observer
	{
	public:
		EnemySystem( eage::ecs::ECSRegistry& registry,
					 eage::ecs::PhysicsSystem& physics_system,
					 eage::ecs::EffectSystem& effect_system,
					 eage::ecs::SceneGraphSystem& scene_graph_system );
		~EnemySystem();

		///
		/// Pre-create a pool of enemy entities for one definition.
		/// Note that this function will perform immediate GPU operations.
		///
		void PreparePool( eage::ecs::RenderSystem& render_system,
						  assets::SceneResourceLoader& resources,
						  const EnemyDefinition& definition,
						  int count,
						  uint64_t root_entity );

		void SetDeathEffect( eage::ecs::ResourceId effect_id );

		/// Destroy all pooled enemy entities and drop shared mesh/material creator refs.
		void ReleaseAll();

		///
		/// Activate `count` inactive entities from the named pool.
		/// Returns false if the pool is missing or fewer than `count` could spawn.
		///
		bool Spawn( const std::string& definition_id, int count );

		void Update();

		// PhysicsSystem::Observer interface
		void OnSensorEnter( uint64_t sensor, uint64_t visitor ) override;
		void OnSensorExit( uint64_t sensor, uint64_t visitor ) override;
		void OnCollideBegin( uint64_t entityA, uint64_t entityB ) override;
		void OnCollideEnd( uint64_t entityA, uint64_t entityB ) override;

	private:
		struct EnemyPool
		{
			std::deque<uint64_t> available;
			std::unordered_set<uint64_t> all;
			eage::ecs::ResourceHandle mesh;
			eage::ecs::ResourceHandle material;
		};

		void Despawn( uint64_t entity );
		void ApplyDriftSpawn( uint64_t entity );
		glm::vec2 PickRandomEdgePosition() const;
		void ApplyContactDamage( uint64_t maybe_enemy, uint64_t maybe_player );

		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::PhysicsSystem& mPhysicsSystem;
		eage::ecs::EffectSystem& mEffectSystem;
		eage::ecs::SceneGraphSystem& mSceneGraphSystem;

		std::unordered_map<std::string, EnemyPool> mPools;
		std::unordered_map<uint64_t, std::string> mEntityToPool;

		eage::ecs::ResourceId mDeathEffectId = eage::ecs::INVALID_ID;

		glm::vec2 mScreenTopLeft;
		glm::vec2 mScreenBottomRight;
	};
}

#endif // _VORTEX_ENEMY_SYSTEM_H_
