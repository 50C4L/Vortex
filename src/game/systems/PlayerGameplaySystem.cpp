#include "PlayerGameplaySystem.h"

#include <ecs/components/Audio.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <utility/Logger.h>

#include "../components/HealthComponent.h"
#include "../components/PlayerComponents.h"

using namespace vortex;
using namespace utility;


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
		// Process incoming damage
		if( mRegistry.HasComponent<HealthComponent>( entity ) )
		{
			auto& health = mRegistry.GetComponent<HealthComponent>( entity );
			if( health.pending_damage > 0.f )
			{
				health.health -= health.pending_damage;
				health.pending_damage = 0.f;
				LOG() << "Player health: " << health.health;

				if( health.IsDead() && player.state == PlayerComponent::State::Alive )
				{
					player.state = PlayerComponent::State::Dead;
					player.thruster_on = false;
					LOG() << "Player has died.";

					// Zero out physics velocity
					if( mRegistry.HasComponent<eage::ecs::PhysicsComponent>( entity ) )
					{
						auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
						physics.QueueSetVelocity( glm::vec2( 0.f, 0.f ) );
						physics.QueueSetAngularVelocity( 0.f );
					}
				}
			}
		}

		UpdateThrusterFX( player, entity );

		// Skip movement and FX when dead
		if( player.state == PlayerComponent::State::Dead )
		{
			continue;
		}

		// We know this entity has PlayerComponent, now check for others
		if( mRegistry.HasComponent<eage::ecs::PhysicsComponent>( entity ) &&
			mRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) )
		{
			auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
			auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
			
			UpdatePlayerMovement( player, physics, transform, delta_time_sec );
		}
	}
}

void
PlayerGameplaySystem::UpdateThrusterFX( PlayerComponent& player_comp, uint64_t entity )
{
	player_comp.thruster_on = player_comp.thruster_on && player_comp.state == PlayerComponent::State::Alive;
	if( player_comp.thruster_fx_entity != 0 &&
		mRegistry.HasComponent<eage::ecs::RenderComponent>( player_comp.thruster_fx_entity ) )
	{
		mRegistry.GetComponent<eage::ecs::RenderComponent>( player_comp.thruster_fx_entity ).visible = player_comp.thruster_on;
	}

	if( mRegistry.HasComponent<AudioEventComponent>( entity ) )
	{
		auto& audio_event = mRegistry.GetComponent<AudioEventComponent>( entity );
		audio_event.QueueEvent( player_comp.thruster_on
			? AudioEventComponent::EventType::Play
			: AudioEventComponent::EventType::Stop );
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
		glm::vec2 velocity_change = glm::vec2( forward.x, forward.y ) * player_comp.thrust_acceleration * delta_time_sec;
		physics_comp.QueueAddVelocity( velocity_change );
	}
}
