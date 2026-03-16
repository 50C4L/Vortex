#ifndef _VORTEX_BULLET_SYSTEM_H_
#define _VORTEX_BULLET_SYSTEM_H_

#include <cstdint>
#include <deque>
#include <unordered_map>

#include <glm/glm.hpp>

#include <ecs/ResourceManager.h>
#include <ecs/systems/PhysicsSystem.h>

namespace eage::ecs
{
	class ECSRegistry;
	class RenderSystem;
}

namespace vortex
{
	using BulletPoolId = uint32_t;

	struct BulletPoolConfig
	{
		float damage = 10.f;
		float collider_radius = 5.f;
		float mesh_width = 10.f;
		float mesh_height = 10.f;
		eage::ecs::ResourceId material_id = 0;
		uint16_t category_bits = 0x0006;
		uint16_t mask_bits = 0x0005;
		glm::vec2 uv_min = glm::vec2( 0.f, 0.f );
		glm::vec2 uv_max = glm::vec2( 1.f, 1.f );
	};

	///
	/// BulletSystem: Manages bullet pools, firing, hit detection, and despawning.
	/// Callers call PreparePool() to register a bullet type and receive a BulletPoolId,
	/// then Fire() to shoot a bullet from that pool.
	///
	class BulletSystem final : public eage::ecs::PhysicsSystem::Observer
	{
	public:
		BulletSystem( eage::ecs::ECSRegistry& registry, eage::ecs::RenderSystem& render_system,
					  eage::ecs::PhysicsSystem& physics_system );
		~BulletSystem();

		///
		/// Pre-create and set up a pool of bullet entities for a given bullet type.
		/// Must be called before Fire() with the returned ID.
		/// Note: performs immediate GPU operations.
		///
		BulletPoolId PreparePool( const BulletPoolConfig& config, int count, uint64_t root_entity );

		///
		/// Fire a bullet from the given pool at the given world position and direction.
		/// Silently skips if the pool is exhausted.
		///
		void Fire( BulletPoolId pool_id, glm::vec2 position, glm::vec2 direction, float speed );

		///
		/// Per-frame update: checks alive bullets for out-of-bounds and despawns them.
		///
		void Update();

		// PhysicsSystem::Observer interface
		void OnSensorEnter( uint64_t sensor, uint64_t visitor ) override;
		void OnSensorExit( uint64_t sensor, uint64_t visitor ) override;
		void OnCollideBegin( uint64_t entityA, uint64_t entityB ) override;
		void OnCollideEnd( uint64_t entityA, uint64_t entityB ) override;

	private:
		void DespawnBullet( uint64_t bullet_entity );

		eage::ecs::ECSRegistry& mRegistry;
		eage::ecs::RenderSystem& mRenderSystem;
		eage::ecs::PhysicsSystem& mPhysicsSystem;

		BulletPoolId mNextPoolId = 1;
		std::unordered_map<BulletPoolId, std::deque<uint64_t>> mPools;
		std::unordered_map<uint64_t, BulletPoolId> mEntityToPool;

		glm::vec2 mScreenTopLeft;
		glm::vec2 mScreenBottomRight;
	};
}

#endif // _VORTEX_BULLET_SYSTEM_H_
