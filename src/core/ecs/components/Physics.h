#ifndef _EAGE_ECS_COMPONENTS_PHYSICS_H_
#define _EAGE_ECS_COMPONENTS_PHYSICS_H_

#include <ecs/ResourceManager.h>
#include <glm/glm.hpp>

namespace eage::ecs
{
	struct PhysicsComponent 
	{
		eage::ecs::ResourceId body_id = 0;
		
		enum class BodyType
		{
			STATIC,
			DYNAMIC,
			KINEMATIC
		} body_type = BodyType::STATIC;

		bool is_sensor = false;

		// If true, the TransformComponent will be updated from the physics body each frame
		// If false, the TransformComponent will only be used to set the initial position of the body
		bool sync_transform_from_body = false;

		// Maximum linear velocity in pixels/second. 0 means no limit.
		float max_linear_velocity = 0.0f;

		// Physics Events
		enum class EventType
		{
			ApplyForce,
			ApplyImpulse,
			ApplyTorque,
			ApplyAngularImpulse,
			SetVelocity,
			SetAngularVelocity,
			SetPosition,
			SetRotation,
			AddVelocity
		};

		struct PhysicsEvent
		{
			EventType type;
			glm::vec2 vector_data{ 0.0f, 0.0f };  // For forces, impulses, velocities, positions
			float scalar_data = 0.0f;             // For torques, angular values, rotations
			bool wake_body = true;                // Whether to wake the body when applying
		};

		std::vector<PhysicsEvent> pending_events;

		// Convenience methods for queueing events
		void QueueForce( const glm::vec2& force, bool wake = true )
		{
			pending_events.push_back({ EventType::ApplyForce, force, 0.0f, wake });
		}

		void QueueImpulse( const glm::vec2& impulse, bool wake = true )
		{
			pending_events.push_back({ EventType::ApplyImpulse, impulse, 0.0f, wake });
		}

		void QueueTorque( float torque, bool wake = true )
		{
			pending_events.push_back({ EventType::ApplyTorque, glm::vec2(0.0f), torque, wake });
		}

		void QueueAngularImpulse( float angular_impulse, bool wake = true )
		{
			pending_events.push_back({ EventType::ApplyAngularImpulse, glm::vec2(0.0f), angular_impulse, wake });
		}

		void QueueSetVelocity( const glm::vec2& velocity )
		{
			pending_events.push_back({ EventType::SetVelocity, velocity, 0.0f, true });
		}

		void QueueSetAngularVelocity( float angular_velocity )
		{
			pending_events.push_back({ EventType::SetAngularVelocity, glm::vec2(0.0f), angular_velocity, true });
		}

		void QueueSetPosition( const glm::vec2& position )
		{
			pending_events.push_back({ EventType::SetPosition, position, 0.0f, false });
		}

		void QueueSetRotation( float rotation_radians )
		{
			pending_events.push_back({ EventType::SetRotation, glm::vec2(0.0f), rotation_radians, false });
		}

		void QueueAddVelocity( const glm::vec2& velocity_change )
		{
			pending_events.push_back({ EventType::AddVelocity, velocity_change, 0.0f, true });
		}

		void ClearEvents()
		{
			pending_events.clear();
		}
	};

	struct BoxColliderComponent 
	{
		float width = 1.0f;
		float height = 1.0f;
		glm::vec2 offset{ 0.0f, 0.0f };

		uint16_t category_bits = 0x0001;	// Which category this collider belongs to
		uint16_t mask_bits = 0xFFFF;		// Which categories this collider collides with
		int16_t group_index = 0;			// Collision filtering group. Colliders with the same positive group index always collide,
											// those with the same negative group index never collide. 0 means no group.
	};

	struct CircleColliderComponent 
	{
		float radius = 0.5f;
		glm::vec2 offset{ 0.0f, 0.0f };

		uint16_t category_bits = 0x0001;	// Which category this collider belongs to
		uint16_t mask_bits = 0xFFFF;		// Which categories this collider collides with
		int16_t group_index = 0;			// Collision filtering group. Colliders with the same positive group index always collide,
											// those with the same negative group index never collide. 0 means no group.
	};
}

#endif // _EAGE_ECS_COMPONENTS_PHYSICS_H_