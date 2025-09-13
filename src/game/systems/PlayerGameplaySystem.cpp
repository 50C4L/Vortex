#include "PlayerGameplaySystem.h"

#include <ecs/components/Audio.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>

#include "../components/PlayerComponents.h"

using namespace vortex;

PlayerGameplaySystem::PlayerGameplaySystem( eage::ecs::ECSRegistry& registry )
	: mRegistry(registry)
{
}

PlayerGameplaySystem::~PlayerGameplaySystem() 
{
}

void
PlayerGameplaySystem::Update( float delta_time_sec )
{
	// Get only entities with PlayerComponent (much smaller set)
	for( auto& [entity, player] : mRegistry.GetComponentMap<PlayerComponent>() )
	{
		// We know this entity has PlayerComponent, now check for others
		if( mRegistry.HasComponent<eage::ecs::PhysicsComponent>( entity ) &&
			mRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) )
		{
			auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
			auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
			
			UpdatePlayerMovement( player, physics, transform, delta_time_sec );

			if( player.thruster_fx_entity != 0 &&
			 	mRegistry.HasComponent<eage::ecs::RenderComponent>( player.thruster_fx_entity ) )
			{
				auto& thruster_render = mRegistry.GetComponent<eage::ecs::RenderComponent>( player.thruster_fx_entity );
				thruster_render.visible = player.thruster_on;
			}

			// Audio logic specific to player thrust
			if( mRegistry.HasComponent<AudioEventComponent>( entity ) )
			{
				auto& audio_event = mRegistry.GetComponent<AudioEventComponent>( entity );
				
				if( player.thruster_on )
				{
					audio_event.QueueEvent( AudioEventComponent::EventType::Play );
				}
				else
				{
					audio_event.QueueEvent( AudioEventComponent::EventType::Stop );
				}
			}
		}
	}
}

void
PlayerGameplaySystem::UpdatePlayerMovement( PlayerComponent& player_comp, 
											eage::ecs::PhysicsComponent& physics_comp,
											eage::ecs::TransformComponent& transform_comp,
											float delta_time_sec )
{
	// Rotation
	if( player_comp.turning_left )
	{
		physics_comp.QueueSetAngularVelocity( player_comp.rotation_speed ); // Convert to radians
	} 
	else if( player_comp.turning_right )
	{
		physics_comp.QueueSetAngularVelocity( -player_comp.rotation_speed ); // Convert to radians
	}
	else
	{
		physics_comp.QueueSetAngularVelocity( 0.0f );
	}

	glm::vec3 forward = transform_comp.rotation * player_comp.forward;

	// Thrust
	if( player_comp.thruster_on )
	{
		glm::vec2 thrust_force = glm::vec2(forward.x, forward.y) * player_comp.thrust_acceleration;
		physics_comp.QueueForce( thrust_force );
	}
}
