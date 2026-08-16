#ifndef _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_
#define _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_

#include <memory>
#include <cstdint>
#include <unordered_set>

#include <glm/glm.hpp>

#include <ecs/ECS.h>
#include <ecs/ResourceStore.h>
#include <physics/PhysicsEventListener.h>

namespace eage::physics
{
	class PhysicsEngine;
	class PhysicsBody;
}

namespace eage::ecs
{
	///
	/// PhysicsSystem: Manages physics bodies and collision components
	///
	class PhysicsSystem final : public eage::physics::PhysicsEventListener, public ECSRegistry::Observer
	{
	public:
		class Observer
		{
		public:
			virtual void OnSensorEnter( uint64_t sensor, uint64_t visitor ) = 0;
			virtual void OnSensorExit( uint64_t sensor, uint64_t visitor ) = 0;

			virtual void OnCollideBegin( uint64_t entityA, uint64_t entityB ) = 0;
			virtual void OnCollideEnd( uint64_t entityA, uint64_t entityB ) = 0;
		};

		PhysicsSystem( ECSRegistry& ecs_registry );
		~PhysicsSystem();

		///
		/// Initialize the physics system and world
		///
		void Initialize( glm::vec2 gravity, float pixels_per_meter );

		///
		/// Update the physics simulation and sync transforms
		///
		void Update( float dt );

		///
		/// Shutdown the physics system and clean up resources
		///
		void Shutdown();

		void Subscribe( Observer* observer );
		void Unsubscribe( Observer* observer );

		// PhysicsEventListener interface
		void OnSensorEnter( physics::PhysicsBody* sensor, physics::PhysicsBody* visitor ) override;
		void OnSensorExit( physics::PhysicsBody* sensor, physics::PhysicsBody* visitor ) override;
		void OnCollideBegin( physics::PhysicsBody* bodyA, physics::PhysicsBody* bodyB ) override;
		void OnCollideEnd( physics::PhysicsBody* bodyA, physics::PhysicsBody* bodyB ) override;

		// ECSRegistry::Observer
		void OnEntityDestroying( Entity entity ) override;

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
		ResourceStore<std::unique_ptr<eage::physics::PhysicsBody>> mBodyStore;
		float mPixelsPerMeter = 1.f;

		std::unordered_set<Observer*> mObservers;
	};
}

#endif // _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_
