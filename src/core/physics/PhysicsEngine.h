#ifndef _EAGE_PHYSICS_ENGINE_H_
#define _EAGE_PHYSICS_ENGINE_H_

#include <memory>
#include <chrono>

#include <box2d/box2d.h>
#include <glm/glm.hpp>

namespace eage::physics
{
	class PhysicsBody
	{
	public:
		PhysicsBody(b2BodyId body_id);
		~PhysicsBody();

		b2BodyId mBodyId;
	};

	///
	/// PhysicsEngine: Manages the physics simulation and world state
	/// Underlying ownership will be released via RAII
	///
	class PhysicsEngine
	{
	public:
		PhysicsEngine();
		~PhysicsEngine();

		void CreateWorld( glm::vec2 gravity = glm::vec2(0.0f, -9.81f) );

		std::unique_ptr<PhysicsBody> CreateBody( const b2BodyDef& body_def );

		void AddCircleColliderToBody( PhysicsBody& body, float radius, glm::vec2 offset = glm::vec2(0.0f, 0.0f) );

		void AddBoxColliderToBody( PhysicsBody& body, float width, float height, glm::vec2 offset = glm::vec2(0.0f, 0.0f) );

		void UpdateBodyTransform( PhysicsBody& body, glm::vec2 position, b2Rot rotation );

		void Update();

	private:
		b2WorldId mWorldId;
		std::chrono::time_point<std::chrono::steady_clock> mLastUpdateTime;
	};
}

#endif // _EAGE_PHYSICS_ENGINE_H_