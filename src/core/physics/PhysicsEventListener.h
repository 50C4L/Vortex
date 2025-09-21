#ifndef _EAGE_PHYSICS_EVENT_LISTENER_H_
#define _EAGE_PHYSICS_EVENT_LISTENER_H_

namespace eage::physics
{
	class PhysicsBody;
	///
	/// PhysicsEventListener: Interface for handling physics events such as sensor interactions
	///
	class PhysicsEventListener
	{
	public:
		virtual void OnSensorEnter( PhysicsBody* sensor, PhysicsBody* visitor) = 0;
		virtual void OnSensorExit( PhysicsBody* sensor, PhysicsBody* visitor) = 0;
	};
}

#endif // _EAGE_PHYSICS_EVENT_LISTENER_H_