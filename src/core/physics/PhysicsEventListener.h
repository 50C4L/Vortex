#ifndef _EAGE_PHYSICS_EVENT_LISTENER_H_
#define _EAGE_PHYSICS_EVENT_LISTENER_H_

namespace eage::physics
{
	class PhysicsBody;
	///
	/// PhysicsEventListener: Interface for handling physics events such as sensor interactions and collisions
	///
	class PhysicsEventListener
	{
	public:
		virtual void OnSensorEnter( PhysicsBody* sensor, PhysicsBody* visitor ) = 0;
		virtual void OnSensorExit( PhysicsBody* sensor, PhysicsBody* visitor ) = 0;

		virtual void OnCollideBegin( PhysicsBody* bodyA, PhysicsBody* bodyB ) = 0;
		virtual void OnCollideEnd( PhysicsBody* bodyA, PhysicsBody* bodyB ) = 0;
	};
}

#endif // _EAGE_PHYSICS_EVENT_LISTENER_H_