#include "WarpSystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <utility/Logger.h>

#include "../components/GameGenericComponents.h"

using namespace vortex;
using namespace utility;

WarpSystem::WarpSystem( eage::ecs::ECSRegistry& registry, eage::ecs::PhysicsSystem& physics_system )
	: mRegistry( registry )
	, mPhysicsSystem( physics_system )
{
	mPhysicsSystem.Subscribe( this );
}

WarpSystem::~WarpSystem()
{
	mPhysicsSystem.Unsubscribe( this );
}

void
WarpSystem::SetScreenEntity( uint64_t screen_entity )
{
	if( !mRegistry.HasComponent<WarpBoundaryComponent>( screen_entity ) )
	{
		LOG_ERROR() << "WarpSystem: Screen entity has no WarpBoundaryComponent.";
		return;
	}
	mScreenEntity = screen_entity;
}

void
WarpSystem::OnSensorEnter( uint64_t sensor, uint64_t visitor )
{
	if( mScreenEntity == 0 ||
		sensor != mScreenEntity || 
		!mRegistry.HasComponent<WarpComponent>( visitor ) )
	{
		return;
	}

	if( !mRegistry.HasComponent<eage::ecs::TransformComponent>( visitor ) )
	{
		LOG_ERROR() << "WarpSystem: Visitor entity has no TransformComponent.";
		return; // No transform component found
	}
}

void
WarpSystem::OnSensorExit( uint64_t sensor, uint64_t visitor )
{
	if( mScreenEntity == 0 ||
		sensor != mScreenEntity || 
		!mRegistry.HasComponent<WarpComponent>( visitor ) )
	{
		return;
	}

	if( !mRegistry.HasComponent<eage::ecs::TransformComponent>( visitor ) )
	{
		LOG_ERROR() << "WarpSystem: Visitor entity has no TransformComponent.";
		return; // No transform component found
	}

	auto& warp_boundary = mRegistry.GetComponent<WarpBoundaryComponent>( sensor );
	auto& visitor_physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( visitor );
	auto& visitor_transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( visitor );

	// Get current position
	glm::vec2 current_pos = glm::vec2( visitor_transform.position.x, visitor_transform.position.y );

	// Calculate wrap-around position using component data
	glm::vec2 new_pos = CalculateWrapPosition( current_pos, warp_boundary );
	
	// Queue teleportation via physics events
	visitor_physics.QueueSetPosition( new_pos );
}

glm::vec2
WarpSystem::CalculateWrapPosition( const glm::vec2& exit_pos, const WarpBoundaryComponent& boundary )
{
	glm::vec2 new_pos = exit_pos;
	
	// Wrap horizontally
	if( exit_pos.x > boundary.right )
	{
		new_pos.x = boundary.left + boundary.wrap_offset;
	}
	else if( exit_pos.x < boundary.left )
	{
		new_pos.x = boundary.right - boundary.wrap_offset;
	}
	
	// Wrap vertically
	if( exit_pos.y > boundary.top )
	{
		new_pos.y = boundary.bottom + boundary.wrap_offset;
	}
	else if( exit_pos.y < boundary.bottom )
	{
		new_pos.y = boundary.top - boundary.wrap_offset;
	}
	
	return new_pos;
}

void
WarpSystem::OnCollideBegin( uint64_t entityA, uint64_t entityB )
{
}

void
WarpSystem::OnCollideEnd( uint64_t entityA, uint64_t entityB )
{
}
