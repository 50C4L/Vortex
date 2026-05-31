#ifndef _EAGE_PHYSICS_ENGINE_H_
#define _EAGE_PHYSICS_ENGINE_H_

#include <memory>

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace eage::physics
{
	class PhysicsEventListener;

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
		struct CollisionFilter
		{
			uint16_t category_bits = 0x0001; // Which category this collider belongs to
			uint16_t mask_bits = 0xFFFF;     // Which categories this collider collides with
			int16_t group_index = 0;         // Collision filtering group. Colliders with the same positive group index always collide,
											 // those with the same negative group index never collide. 0 means no group.
		};

		PhysicsEngine();
		~PhysicsEngine();

		///
		/// Create a new physics world with specified gravity
		///
		void CreateWorld( glm::vec2 gravity = glm::vec2(0.0f, -9.81f) );

		///
		/// Create a new physics body in the world with specified body definition
		///
		std::unique_ptr<PhysicsBody> CreateBody( const b2BodyDef& body_def );

		///
		/// Add a circle collider to the specified body
		///
		void AddCircleColliderToBody( PhysicsBody& body, CollisionFilter filter, float radius, bool is_sensor, glm::vec2 offset = glm::vec2(0.0f, 0.0f) );

		///
		/// Add a box collider to the specified body
		///
		void AddBoxColliderToBody( PhysicsBody& body, CollisionFilter filter, float width, float height, bool is_sensor, glm::vec2 offset = glm::vec2(0.0f, 0.0f) );

		///
		/// Update the transform of the specified body
		///
		void UpdateBodyTransform( PhysicsBody& body, glm::vec2 position, b2Rot rotation );

		///
		/// Step the physics simulation forward
		///
		void Update( float dt );

		///
		/// Get the current transform of the specified body
		///
		struct PhysicsBodyTransform
		{
			glm::vec3 position = glm::vec3(0.0f);
			glm::quat rotation = glm::quat();
		};
		PhysicsBodyTransform GetBodyTransform( const PhysicsBody& body );

		///
		/// Set the event listener for physics events
		///
		/// Caller is responsible for ensuring the listener remains valid while set
		///
		void SetEventListener( PhysicsEventListener* listener );
		void ClearEventListener();

		void* GetUserData( const PhysicsBody& body ) const;

	private:
		void ProcessSensorEvents();
		void ProcessContactEvents();

		b2WorldId mWorldId;
		b2DebugDraw mDebugDraw;
		float mAccumulator = 0.f;

		PhysicsEventListener* mEventListener = nullptr;
	};
}

#endif // _EAGE_PHYSICS_ENGINE_H_