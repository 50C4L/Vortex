#ifndef _VORTEX_ASTEROID_GAMEPLAY_SYSTEM_H_
#define _VORTEX_ASTEROID_GAMEPLAY_SYSTEM_H_

#include <deque>
#include <unordered_set>
#include <glm/glm.hpp>

#include <ecs/ResourceManager.h>
#include <ecs/systems/PhysicsSystem.h>

namespace assets
{
	class SceneResourceLoader;
}

namespace eage::ecs
{
	class ECSRegistry;
	class EffectSystem;
	class RenderSystem;
}

namespace vortex
{
	class AsteroidGameplaySystem : public eage::ecs::PhysicsSystem::Observer
	{
	public:
		AsteroidGameplaySystem( eage::ecs::ECSRegistry& registry,
								eage::ecs::PhysicsSystem& physics_system,
								eage::ecs::EffectSystem& effect_system );
		~AsteroidGameplaySystem();

		///
		/// Precreate and setup a number of asteroids
		/// Note that this function will perform immediate GPU operations
		///
		void PrepareAsteroids( eage::ecs::RenderSystem& render_system, assets::SceneResourceLoader& resources, int count, uint64_t root_entity );

		void SetDeathEffect( eage::ecs::ResourceId effect_id );

		void SpawnAsteroid( int count );
		void DespawnAsteroid( uint64_t asteroid_entity );

		int GetKillCount() const { return mKillCount; }

		void Update();

		// PhysicsSystem::Observer interface
		void OnSensorEnter( uint64_t sensor, uint64_t visitor ) override;
		void OnSensorExit( uint64_t sensor, uint64_t visitor ) override;
		void OnCollideBegin( uint64_t entityA, uint64_t entityB ) override;
		void OnCollideEnd( uint64_t entityA, uint64_t entityB ) override;

	private:
		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::PhysicsSystem& mPhysicsSystem;
		eage::ecs::EffectSystem& mEffectSystem;

		eage::ecs::ResourceId mAsteroidMaterialId = 0;
		eage::ecs::ResourceId mAsteroidMeshId = 0;
		uint32_t mAsteroidTextureIndex = 0;
		eage::ecs::ResourceId mDeathEffectId = eage::ecs::INVALID_ID;

		std::deque<uint64_t> mAvailableAsteroids;
		std::unordered_set<uint64_t> mAllAsteroids;

		glm::vec2 mScreenTopLeft;
		glm::vec2 mScreenBottomRight;

		int mKillCount = 0;
	};
}

#endif // _VORTEX_ASTEROID_GAMEPLAY_SYSTEM_H_
