#include "WarpSystem.h"

#include <ecs/ECS.h>

#include "../components/GameGenericComponents.h"

using namespace vortex;

WarpSystem::WarpSystem( eage::ecs::ECSRegistry& registry, eage::ecs::PhysicsSystem& physics_system )
	: mRegistry( registry )
	, mPhysicsSystem( physics_system )
{
	mPhysicsSystem.RegisterObserver( this );
}

WarpSystem::~WarpSystem()
{
	mPhysicsSystem.UnregisterObserver( this );
}

void
WarpSystem::SetScreenEntity( uint64_t screen_entity )
{
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
}
