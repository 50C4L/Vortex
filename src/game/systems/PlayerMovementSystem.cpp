#include "PlayerMovementSystem.h"

#include <ecs/components/Audio.h>
#include <ecs/components/Basics.h>

#include "../components/PlayerComponents.h"

using namespace vortex;

PlayerMovementSystem::PlayerMovementSystem( eage::ecs::ECSRegistry& registry )
	: mRegistry(registry)
{
}

PlayerMovementSystem::~PlayerMovementSystem() 
{
}

void
PlayerMovementSystem::Update( float delta_time_sec )
{
	// Get only entities with PlayerComponent (much smaller set)
	for( auto& [entity, player] : mRegistry.GetComponentMap<PlayerComponent>() )
	{
		// We know this entity has PlayerComponent, now check for others
		if( mRegistry.HasComponent<eage::ecs::Velocity2DComponent>( entity ) &&
			mRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) )
		{
			auto& movement = mRegistry.GetComponent<eage::ecs::Velocity2DComponent>( entity );
			auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
			
			UpdatePlayerMovement(player, movement, transform, delta_time_sec);

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
PlayerMovementSystem::UpdatePlayerMovement( PlayerComponent& player_comp, 
											eage::ecs::Velocity2DComponent& velocity_comp,
											eage::ecs::TransformComponent& transform_comp,
											float delta_time_sec )
{
	// Rotation
	if( player_comp.turning_left )
	{
		velocity_comp.angular_velocity = player_comp.rotation_speed;
	} 
	else if( player_comp.turning_right )
	{
		velocity_comp.angular_velocity = -player_comp.rotation_speed;
	} 
	else
	{
		velocity_comp.angular_velocity = 0.0f;
	}

	glm::vec3 forward = transform_comp.rotation * player_comp.forward;

	// Thrust
	if( player_comp.thruster_on )
	{
		velocity_comp.velocity += forward * player_comp.thrust_acceleration * delta_time_sec;
		
		// Clamp speed
		if( glm::length( velocity_comp.velocity ) > player_comp.max_thrust_speed )
		{
			velocity_comp.velocity = glm::normalize( velocity_comp.velocity ) * player_comp.max_thrust_speed;
		}
	}
	
	// Apply movement
	if( velocity_comp.angular_velocity != 0.0f )
	{
		float angle_rad = glm::radians( velocity_comp.angular_velocity * delta_time_sec );
		glm::quat delta_rot = glm::angleAxis( angle_rad, glm::vec3( 0.0f, 0.0f, 1.0f ) );
		transform_comp.SetRotation( glm::normalize( delta_rot * transform_comp.rotation ) );
	}
	transform_comp.position += velocity_comp.velocity * delta_time_sec;
	transform_comp.MarkDirty();
}
