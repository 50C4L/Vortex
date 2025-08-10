#include "ShipControlSystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>

#include "../components/ShipStateComponents.h"

using namespace vortex;
using namespace eage::ecs;

ShipControlSystem::ShipControlSystem( ECSRegistry& ecs_registry, Entity ship_entity )
	: mECSRegistry( ecs_registry )
	, mShipEntity( ship_entity )
{
}

ShipControlSystem::~ShipControlSystem()
{
}

void
ShipControlSystem::Update( float delta_time )
{
	// Get components
	auto& transform = mECSRegistry.GetComponent<eage::ecs::TransformComponent>( mShipEntity );
	auto& velocity = mECSRegistry.GetComponent<eage::ecs::VelocityComponent>( mShipEntity );

	// Calculate forward direction from rotation
	glm::vec3 forward = transform.rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

	// Thrust: apply acceleration if thrust is on
	if( mIsThrustOn )
	{
		float thrust_acceleration = 100.0f; // You can make this configurable
		velocity.velocity += forward * thrust_acceleration * ( delta_time / 1000.0f );
	}

	// Rotation: rotate around Z axis
	if( mRotateSpeed != 0.0f )
	{
		float angle_rad = glm::radians( mRotateSpeed * ( delta_time / 1000.0f ) );
		glm::quat delta_rot = glm::angleAxis( angle_rad, glm::vec3( 0.0f, 0.0f, 1.0f ) );
		transform.rotation = glm::normalize( delta_rot * transform.rotation );
	}

	// Update position
	transform.position += velocity.velocity * (delta_time / 1000.0f);

	// Now transform contains the updated position and rotation for rendering
}

void
ShipControlSystem::Thrust( bool on )
{
	auto& ship_state = mECSRegistry.GetComponent<components::ShipStateComponent>( mShipEntity );
	ship_state.is_thrust_on = on;
	mIsThrustOn = on;
}

void
ShipControlSystem::Rotate( float angle )
{
	mRotateSpeed = angle; // Set the rotation speed, positive for right rotation, negative for left rotation
}