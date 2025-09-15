#ifndef _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_
#define _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_

#include <memory>
#include <cstdint>

#include <glm/glm.hpp>

#include <ecs/ResourceManager.h>

namespace eage::physics
{
	class PhysicsEngine;
	class PhysicsBody;
}

namespace eage::ecs
{
	class ECSRegistry;

	///
	/// PhysicsSystem: Manages physics bodies and collision components
	///
	class PhysicsSystem
	{
	public:
		PhysicsSystem( ECSRegistry& ecs_registry );
		~PhysicsSystem();

		///
		/// Initialize the physics system and world
		///
		void Initialize( glm::vec2 gravity, float pixels_per_meter );

		///
		/// Update the physics simulation and sync transforms
		///
		void Update();

		///
		/// Shutdown the physics system and clean up resources
		///
		void Shutdown();

	private:
		void CreateCollisionBodyFromComponents( uint64_t entity );
		void SyncTransformFromBodies( uint64_t entity );

		// Conversion utilities
		float PixelsToMeters( float pixels ) const { return pixels / mPixelsPerMeter; }
		float MetersToPixels( float meters ) const { return meters * mPixelsPerMeter; }
		glm::vec2 PixelsToMeters( const glm::vec2& pixels ) const { return pixels / mPixelsPerMeter; }
		glm::vec2 MetersToPixels( const glm::vec2& meters ) const { return meters * mPixelsPerMeter; }
		glm::vec3 MetersToPixels( const glm::vec3& pixels ) const { return pixels * mPixelsPerMeter; }

		ECSRegistry& mECSRegistry;
		std::unique_ptr<eage::physics::PhysicsEngine> mPhysicsEngine;
		ResourceManager<std::unique_ptr<eage::physics::PhysicsBody>> mBodyManager;
		float mPixelsPerMeter = 1.f;
	};
}

#endif // _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_