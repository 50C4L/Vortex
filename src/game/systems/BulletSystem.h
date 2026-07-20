#ifndef _VORTEX_BULLET_SYSTEM_H_
#define _VORTEX_BULLET_SYSTEM_H_

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

#include <ecs/ResourceManager.h>
#include <ecs/systems/PhysicsSystem.h>

namespace eage::ecs
{
	class AnimationSystem;
	class ECSRegistry;
	class RenderSystem;
}

namespace vortex
{
	using BulletPoolId = uint32_t;

	struct BulletPoolConfig
	{
		float damage = 10.f;
		float collider_radius = 4.f;
		float mesh_width = 8.f;
		float mesh_height = 8.f;
		eage::ecs::ResourceId material_id = 0;
		uint16_t category_bits = 0x0006;
		uint16_t mask_bits = 0x0005;
		uint32_t texture_index = 0;
		float fire_interval = 0.f; // Minimum seconds between shots; 0 = unlimited
		float lifetime_sec = 0.f;
		eage::ecs::ResourceId clip_id = eage::ecs::INVALID_ID;
	};

	///
	/// BulletSystem: Manages bullet pools, firing, hit detection, and despawning.
	/// Callers call PreparePool() to register a bullet type and receive a BulletPoolId,
	/// then Fire() to shoot a bullet from that pool.
	///
	class BulletSystem final : public eage::ecs::PhysicsSystem::Observer
	{
	public:
		BulletSystem( eage::ecs::ECSRegistry& registry, eage::ecs::PhysicsSystem& physics_system,
					  eage::ecs::AnimationSystem& animation_system );
		~BulletSystem();

		///
		/// Pre-create and set up a pool of bullet entities for a given bullet type.
		/// Must be called before Fire() with the returned ID.
		/// Note: performs immediate GPU operations.
		///
		BulletPoolId PreparePool( eage::ecs::RenderSystem& render_system, const BulletPoolConfig& config, int count, uint64_t root_entity );

		///
		/// Fire a bullet from the given pool at the given world position and direction.
		/// Returns true if a bullet was spawned, false if rate-limited or pool exhausted.
		///
		bool Fire( BulletPoolId pool_id, glm::vec2 position, glm::vec2 direction, float speed );

		///
		/// Per-frame update: expires bullets by lifetime and despawns finished dying bullets.
		///
		void Update( float dt );

		// PhysicsSystem::Observer interface
		void OnSensorEnter( uint64_t sensor, uint64_t visitor ) override;
		void OnSensorExit( uint64_t sensor, uint64_t visitor ) override;
		void OnCollideBegin( uint64_t entityA, uint64_t entityB ) override;
		void OnCollideEnd( uint64_t entityA, uint64_t entityB ) override;

	private:
		void BeginHitReaction( uint64_t bullet_entity );
		void DespawnBullet( uint64_t bullet_entity );

		eage::ecs::ECSRegistry& mRegistry;
		eage::ecs::PhysicsSystem& mPhysicsSystem;
		eage::ecs::AnimationSystem& mAnimationSystem;

		BulletPoolId mNextPoolId = 1;
		std::unordered_map<BulletPoolId, std::deque<uint64_t>> mPools;
		std::unordered_map<uint64_t, BulletPoolId> mEntityToPool;
		std::unordered_map<BulletPoolId, float> mPoolFireInterval;
		std::unordered_map<BulletPoolId, std::chrono::steady_clock::time_point> mPoolLastFireTime;

		glm::vec2 mScreenBottomRight;
	};
}

#endif // _VORTEX_BULLET_SYSTEM_H_
