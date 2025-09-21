#include "PhysicsSystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <physics/PhysicsEngine.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace eage::physics;
using namespace utility;

namespace
{
	b2Rot quat_to_b2rot( const glm::quat& q )
	{
		// Extract Z-axis rotation from quaternion
		float angle = 2.0f * atan2( q.z, q.w );
		b2Rot rot;
		rot.c = cos(angle);
		rot.s = sin(angle);
		return rot;
	}
}

PhysicsSystem::PhysicsSystem( ECSRegistry& ecs_registry )
	: mECSRegistry( ecs_registry )
	, mPhysicsEngine( std::make_unique<eage::physics::PhysicsEngine>() )
{
}

PhysicsSystem::~PhysicsSystem()
{
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
PhysicsSystem::Update() 
{
	for( auto& [entity, physics] : mECSRegistry.GetComponentMap<PhysicsComponent>() )
	{
		if( !mECSRegistry.HasComponent<TransformComponent>( entity ) )
		{
			continue; // No collider component found
		}

		if( physics.body_id == INVALID_ID )
		{
			CreateCollisionBodyFromComponents( entity );
		}

		// Process physics events
		if( physics.body_id != INVALID_ID )
		{
			auto body = mBodyManager.Get( physics.body_id );
			if( body )
			{
				for( const auto& event : physics.pending_events )
				{
					switch( event.type )
					{
						case PhysicsComponent::EventType::ApplyForce:
						{
							glm::vec2 physics_force = PixelsToMeters( event.vector_data );
							b2Body_ApplyForceToCenter( body->mBodyId, b2Vec2{ physics_force.x, physics_force.y }, event.wake_body );
						}
							break;
						case PhysicsComponent::EventType::ApplyImpulse:
							b2Body_ApplyLinearImpulseToCenter( body->mBodyId, b2Vec2{ event.vector_data.x, event.vector_data.y }, event.wake_body );
							break;
						case PhysicsComponent::EventType::ApplyTorque:
							b2Body_ApplyTorque( body->mBodyId, event.scalar_data, event.wake_body );
							break;
						case PhysicsComponent::EventType::ApplyAngularImpulse:
							b2Body_ApplyAngularImpulse( body->mBodyId, event.scalar_data, event.wake_body );
							break;
						case PhysicsComponent::EventType::SetVelocity:
							b2Body_SetLinearVelocity( body->mBodyId, b2Vec2{ event.vector_data.x, event.vector_data.y } );
							break;
						case PhysicsComponent::EventType::SetAngularVelocity:
							b2Body_SetAngularVelocity( body->mBodyId, event.scalar_data );
							break;
						case PhysicsComponent::EventType::SetPosition:
						{
							auto current_rot = b2Body_GetRotation( body->mBodyId );
							glm::vec2 physics_position = PixelsToMeters( event.vector_data );
							b2Body_SetTransform( body->mBodyId, b2Vec2{ physics_position.x, physics_position.y }, current_rot );
							break;
						}
						case PhysicsComponent::EventType::SetRotation:
						{
							auto current_pos = b2Body_GetPosition( body->mBodyId );
							b2Rot new_rot = b2MakeRot( event.scalar_data);
							b2Body_SetTransform( body->mBodyId, current_pos, new_rot );
							break;
						}
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

		SyncTransformFromBodies( entity );
	}

	mPhysicsEngine->Update();
}

void
PhysicsSystem::Shutdown()
{
	// Cleanup logic would go here
	mBodyManager.Clear();
	mPhysicsEngine->ClearEventListener();
}

void
PhysicsSystem::RegisterObserver( Observer* observer )
{
	mObservers.insert( observer );
}

void
PhysicsSystem::UnregisterObserver( Observer* observer )
{
	mObservers.erase( observer );
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
PhysicsSystem::CreateCollisionBodyFromComponents( uint64_t entity )
{
	auto& physics = mECSRegistry.GetComponent<PhysicsComponent>( entity );
	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );

	// Convert pixel position to meters for Box2D
	glm::vec2 physics_position = PixelsToMeters( glm::vec2(transform.position.x, transform.position.y) );

	// Create Box2D body definition
	b2BodyDef body_def = b2DefaultBodyDef();
	switch( physics.body_type )
	{
		case PhysicsComponent::BodyType::STATIC:
			body_def.type = b2_staticBody;
			break;
		case PhysicsComponent::BodyType::DYNAMIC:
			body_def.type = b2_dynamicBody;
			break;
		case PhysicsComponent::BodyType::KINEMATIC:
			body_def.type = b2_kinematicBody;
			break;
		default:
			LOG_ERROR() << "Unknown body type for entity " << entity << ". Defaulting to STATIC.";
			body_def.type = b2_staticBody;
			break;
	}
	body_def.position = b2Vec2{ physics_position.x, physics_position.y };
	body_def.rotation = quat_to_b2rot( transform.rotation );

	// Store entity ID in userData by casting it to a pointer
	body_def.userData = reinterpret_cast<void*>( static_cast<uintptr_t>( entity ) );

	auto physics_body = mPhysicsEngine->CreateBody( body_def );

	// Add circle collider
	if( mECSRegistry.HasComponent<CircleColliderComponent>( entity ) )
	{
		auto& circle_collider = mECSRegistry.GetComponent<CircleColliderComponent>( entity );
		float physics_radius = PixelsToMeters( circle_collider.radius );
		glm::vec2 physics_offset = PixelsToMeters( circle_collider.offset );

		mPhysicsEngine->AddCircleColliderToBody( 
			*physics_body, { circle_collider.category_bits, circle_collider.mask_bits, circle_collider.group_index },
			physics_radius, physics.is_sensor, physics_offset );
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
			physics_width, physics_height, physics.is_sensor, physics_offset );
	}

	// @todo more collider types

	LOG() << "Created body " << physics_body->mBodyId.index1 << " for entity " << entity;

	// Store the body in the resource manager and save the ID in the component
	physics.body_id = mBodyManager.Store( std::move( physics_body ) );
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

	auto body = mBodyManager.Get( physics.body_id );
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
