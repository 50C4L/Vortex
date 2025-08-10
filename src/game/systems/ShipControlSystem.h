#ifndef _VORTEX_SHIP_CONTROL_SYSTEM_H
#define _VORTEX_SHIP_CONTROL_SYSTEM_H

#include <ecs/ECS.h>

namespace vortex
{
class ShipControlSystem
{
public:
	ShipControlSystem( eage::ecs::ECSRegistry& ecs_registry, eage::ecs::Entity ship_entity );
	~ShipControlSystem();

	void Update( float delta_time );
	void Thrust( bool on );
	void Rotate( float angle );

private:
	eage::ecs::ECSRegistry& mECSRegistry;
	eage::ecs::Entity mShipEntity;

	bool mIsThrustOn = false;
	float mRotateSpeed = 0.0f; // Angle per second, positive for right rotation, negative for left rotation
};

} // namespace vortex

#endif // _VORTEX_SHIP_CONTROL_SYSTEM_H