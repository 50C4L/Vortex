#include "PhysicsSystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <physics/PhysicsEngine.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace eage::physics;
using namespace utility;

PhysicsSystem::PhysicsSystem( ECSRegistry& ecs_registry )
	: mECSRegistry( ecs_registry )
	, mPhysicsEngine( std::make_unique<eage::physics::PhysicsEngine>() )
{
	mECSRegistry.Subscribe( this );
}

PhysicsSystem::~PhysicsSystem()
{
	mECSRegistry.Unsubscribe( this );
	Shutdown();
}

void
PhysicsSystem::Initialize( glm::vec2 gravity, float pixels_per_meter ) 
{
	mPixelsPerMeter = pixels_per_meter;
	mPhysicsEngine->CreateWorld( std::move( gravity ) );
	mPhysicsEngine->SetEventListener( this );
}

void
PhysicsSystem::Update( float dt ) 
{
	if( IsPaused() )
	{
		return;
	}

	for( auto [entity, physics] : mECSRegistry.GetComponentMap<PhysicsComponent>() )
	{
		if( !mECSRegistry.HasComponent<TransformComponent>( entity ) )
		{
			continue; // No transform component found
		}

		if( physics.body_id == INVALID_ID )
		{
			CreateCollisionBodyFromComponents( entity );
		}

		// Inactive bodies with no pending work can be skipped entirely
		if( !physics.active && physics.pending_events.empty() )
		{
			continue;
		}

		// Process physics events (always when pending, so SetActive can re-enable the body)
		if( physics.body_id != INVALID_ID )
		{
			auto body = mBodyStore.Get( physics.body_id );
			if( body )
			{
				for( const auto& event : physics.pending_events )
				{
					switch( event.type )
					{
						case PhysicsComponent::EventType::ApplyForce:
						{
							glm::vec2 physics_force = PixelsToMeters( event.vector_data );
							mPhysicsEngine->ApplyForce( *body, physics_force, event.wake_body );
						}
							break;
						case PhysicsComponent::EventType::ApplyImpulse:
							mPhysicsEngine->ApplyLinearImpulse( *body, event.vector_data, event.wake_body );
							break;
						case PhysicsComponent::EventType::ApplyTorque:
							mPhysicsEngine->ApplyTorque( *body, event.scalar_data, event.wake_body );
							break;
						case PhysicsComponent::EventType::ApplyAngularImpulse:
							mPhysicsEngine->ApplyAngularImpulse( *body, event.scalar_data, event.wake_body );
							break;
						case PhysicsComponent::EventType::SetVelocity:
						{
							glm::vec2 physics_velocity = PixelsToMeters( event.vector_data );
							mPhysicsEngine->SetLinearVelocity( *body, physics_velocity );
						}
							break;
						case PhysicsComponent::EventType::SetAngularVelocity:
							mPhysicsEngine->SetAngularVelocity( *body, event.scalar_data );
							break;
						case PhysicsComponent::EventType::SetPosition:
						{
							glm::vec2 physics_position = PixelsToMeters( event.vector_data );
							mPhysicsEngine->SetPosition( *body, physics_position );
							break;
						}
						case PhysicsComponent::EventType::SetRotation:
						{
							mPhysicsEngine->SetRotation( *body, event.scalar_data );
							break;
						}
						case PhysicsComponent::EventType::AddVelocity:
						{
							glm::vec2 current_velocity = MetersToPixels( mPhysicsEngine->GetLinearVelocity( *body ) );

							glm::vec2 new_velocity = current_velocity + event.vector_data;

							if( physics.max_linear_velocity > 0.0f )
							{
								float new_speed = glm::length( new_velocity );
								if( new_speed > physics.max_linear_velocity )
								{
									new_velocity = glm::normalize( new_velocity ) * physics.max_linear_velocity;
								}
							}

							glm::vec2 physics_velocity = PixelsToMeters( new_velocity );
							mPhysicsEngine->SetLinearVelocity( *body, physics_velocity );
							break;
						}
						case PhysicsComponent::EventType::SetActive:
							if( event.scalar_data > 0.5f )
							{
								physics.active = true;
								mPhysicsEngine->SetBodyEnabled( *body, true );
							}
							else
							{
								// Zero velocity while still enabled (disabled bodies have no BodyState)
								mPhysicsEngine->SetLinearVelocity( *body, glm::vec2( 0.0f, 0.0f ) );
								mPhysicsEngine->SetAngularVelocity( *body, 0.0f );
								physics.active = false;
								mPhysicsEngine->SetBodyEnabled( *body, false );
							}
							break;
						default:
							LOG_ERROR() << "Unknown physics event type for entity " << entity;
							break;
					}
				}
				physics.ClearEvents();
			}
			else
			{
				LOG_ERROR() << "Invalid physic body in PhysicsComponent for entity " << entity;
			}
		}

		if( !physics.active )
		{
			continue; // Body is inactive, skip transform sync
		}

		SyncTransformFromBodies( entity );
	}

	mPhysicsEngine->Update( dt );
}

void
PhysicsSystem::Shutdown()
{
	// Cleanup logic would go here
	mBodyStore.Clear();
	mPhysicsEngine->ClearEventListener();
}

void
PhysicsSystem::Subscribe( Observer* observer )
{
	mObservers.insert( observer );
}

void
PhysicsSystem::Unsubscribe( Observer* observer )
{
	mObservers.erase( observer );
}

void
PhysicsSystem::OnEntityDestroying( Entity entity )
{
	if( !mECSRegistry.HasComponent<PhysicsComponent>( entity ) )
	{
		return;
	}

	auto& physics = mECSRegistry.GetComponent<PhysicsComponent>( entity );
	if( physics.body_id != INVALID_ID )
	{
		mBodyStore.RemoveReference( physics.body_id );
		physics.body_id = INVALID_ID;
	}
}

void
PhysicsSystem::OnSensorEnter( physics::PhysicsBody* sensor, physics::PhysicsBody* visitor )
{
	if( !sensor || !visitor )
	{
		return;
	}

	auto sensor_data = mPhysicsEngine->GetUserData( *sensor );
	auto visitor_data = mPhysicsEngine->GetUserData( *visitor );
	if( !sensor_data || !visitor_data )
	{
		LOG_ERROR() << "Sensor or visitor body has no associated entity.";
		return;
	}

	for( auto* observer : mObservers )
	{
		uint64_t sensor_entity = reinterpret_cast<uint64_t>( sensor_data );
		uint64_t visitor_entity = reinterpret_cast<uint64_t>( visitor_data );
		observer->OnSensorEnter( sensor_entity, visitor_entity );
	}
}

void
PhysicsSystem::OnSensorExit( physics::PhysicsBody* sensor, physics::PhysicsBody* visitor )
{
	if( !sensor || !visitor )
	{
		return;
	}

	auto sensor_data = mPhysicsEngine->GetUserData( *sensor );
	auto visitor_data = mPhysicsEngine->GetUserData( *visitor );
	if( !sensor_data || !visitor_data )
	{
		LOG_ERROR() << "Sensor or visitor body has no associated entity.";
		return;
	}

	for( auto* observer : mObservers )
	{
		uint64_t sensor_entity = reinterpret_cast<uint64_t>( sensor_data );
		uint64_t visitor_entity = reinterpret_cast<uint64_t>( visitor_data );
		observer->OnSensorExit( sensor_entity, visitor_entity );
	}
}

void
PhysicsSystem::OnCollideBegin( physics::PhysicsBody* bodyA, physics::PhysicsBody* bodyB )
{
	if( !bodyA || !bodyB )
	{
		return;
	}

	auto data_a = mPhysicsEngine->GetUserData( *bodyA );
	auto data_b = mPhysicsEngine->GetUserData( *bodyB );
	if( !data_a || !data_b )
	{
		LOG_ERROR() << "Colliding body has no associated entity.";
		return;
	}

	for( auto* observer : mObservers )
	{
		uint64_t entity_a = reinterpret_cast<uint64_t>( data_a );
		uint64_t entity_b = reinterpret_cast<uint64_t>( data_b );
		observer->OnCollideBegin( entity_a, entity_b );
	}
}

void
PhysicsSystem::OnCollideEnd( physics::PhysicsBody* bodyA, physics::PhysicsBody* bodyB )
{
	if( !bodyA || !bodyB )
	{
		return;
	}

	auto data_a = mPhysicsEngine->GetUserData( *bodyA );
	auto data_b = mPhysicsEngine->GetUserData( *bodyB );
	if( !data_a || !data_b )
	{
		LOG_ERROR() << "Colliding body has no associated entity.";
		return;
	}

	for( auto* observer : mObservers )
	{
		uint64_t entity_a = reinterpret_cast<uint64_t>( data_a );
		uint64_t entity_b = reinterpret_cast<uint64_t>( data_b );
		observer->OnCollideEnd( entity_a, entity_b );
	}
}

void
PhysicsSystem::CreateCollisionBodyFromComponents( uint64_t entity )
{
	auto& physics = mECSRegistry.GetComponent<PhysicsComponent>( entity );
	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );

	// Convert pixel position to meters for Box2D
	glm::vec2 physics_position = PixelsToMeters( glm::vec2(transform.position.x, transform.position.y) );

	PhysicsEngine::BodyDefinition body_def;
	switch( physics.body_type )
	{
		case PhysicsComponent::BodyType::STATIC:
			body_def.type = PhysicsEngine::BodyDefinition::BodyType::Static;
			break;
		case PhysicsComponent::BodyType::DYNAMIC:
			body_def.type = PhysicsEngine::BodyDefinition::BodyType::Dynamic;
			break;
		case PhysicsComponent::BodyType::KINEMATIC:
			body_def.type = PhysicsEngine::BodyDefinition::BodyType::Kinematic;
			break;
		default:
			LOG_ERROR() << "Unknown body type for entity " << entity << ". Defaulting to STATIC.";
			body_def.type = PhysicsEngine::BodyDefinition::BodyType::Static;
			break;
	}
	body_def.position = physics_position;
	body_def.rotation = transform.rotation;
	body_def.is_bullet = physics.is_bullet;
	body_def.user_data = reinterpret_cast<void*>( static_cast<uintptr_t>( entity ) );

	auto physics_body = mPhysicsEngine->CreateBody( body_def );

	// Add circle collider
	if( mECSRegistry.HasComponent<CircleColliderComponent>( entity ) )
	{
		auto& circle_collider = mECSRegistry.GetComponent<CircleColliderComponent>( entity );
		float physics_radius = PixelsToMeters( circle_collider.radius );
		glm::vec2 physics_offset = PixelsToMeters( circle_collider.offset );

		mPhysicsEngine->AddCircleColliderToBody( 
			*physics_body, { circle_collider.category_bits, circle_collider.mask_bits, circle_collider.group_index },
			physics_radius, circle_collider.is_sensor, physics_offset );
	}

	// Add box collider
	if( mECSRegistry.HasComponent<BoxColliderComponent>( entity ) )
	{
		auto& box_collider = mECSRegistry.GetComponent<BoxColliderComponent>( entity );
		float physics_width = PixelsToMeters( box_collider.width );
		float physics_height = PixelsToMeters( box_collider.height );
		glm::vec2 physics_offset = PixelsToMeters( box_collider.offset );

		mPhysicsEngine->AddBoxColliderToBody(
			*physics_body, { box_collider.category_bits, box_collider.mask_bits, box_collider.group_index },
			physics_width, physics_height, box_collider.is_sensor, physics_offset );
	}

	// @todo more collider types

	// Store the body in the resource manager and save the ID in the component
	physics.body_id = mBodyStore.Store( std::move( physics_body ) );
}

void
PhysicsSystem::SyncTransformFromBodies( uint64_t entity )
{
	auto& physics = mECSRegistry.GetComponent<PhysicsComponent>( entity );
	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );

	if( physics.body_id == INVALID_ID || !physics.sync_transform_from_body )
	{
		return;
	}

	auto body = mBodyStore.Get( physics.body_id );
	if( !body )
	{
		LOG_ERROR() << "Invalid body ID in PhysicsComponent for entity " << entity;
		return; // Invalid body ID
	}

	auto body_transform = mPhysicsEngine->GetBodyTransform( *body );
	// Convert meters back to pixels for rendering
	glm::vec3 pixel_position = MetersToPixels( body_transform.position );

	transform.SetPosition( pixel_position );
	transform.SetRotation( body_transform.rotation );
}
