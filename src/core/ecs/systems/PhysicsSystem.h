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

		void Initialize( glm::vec2 gravity );
		void Update();
		void Shutdown();

	private:
		void CreateCollisionBodyFromComponents( uint64_t entity );
		void SyncTransformToBodies( uint64_t entity );
		void SyncTransformFromBodies( uint64_t entity );

		ECSRegistry& mECSRegistry;
		std::unique_ptr<eage::physics::PhysicsEngine> mPhysicsEngine;
		ResourceManager<std::unique_ptr<eage::physics::PhysicsBody>> mBodyManager;
	};
}

#endif // _EAGE_ECS_SYSTEMS_PHYSICS_SYSTEM_H_