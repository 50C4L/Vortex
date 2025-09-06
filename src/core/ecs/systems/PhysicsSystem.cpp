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
PhysicsSystem::Initialize( glm::vec2 gravity ) 
{
	mPhysicsEngine->CreateWorld( std::move( gravity ) );
}

void
PhysicsSystem::Update() 
{
	for( auto& [entity, collision] : mECSRegistry.GetComponentMap<CollisionComponent>() )
	{
		if( !mECSRegistry.HasComponent<TransformComponent>( entity ) )
		{
			continue; // No collider component found
		}

		if( collision.body_id == INVALID_ID )
		{
			CreateCollisionBodyFromComponents( entity );
		}

		SyncTransformToStaticBodies( entity );
	}

	mPhysicsEngine->Update();
}

void
PhysicsSystem::Shutdown()
{
	// Cleanup logic would go here
}

void
PhysicsSystem::CreateCollisionBodyFromComponents( uint64_t entity )
{
	auto& collision = mECSRegistry.GetComponent<CollisionComponent>( entity );
	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );

	// Create Box2D body definition
	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = collision.is_static ? b2_staticBody : b2_dynamicBody;
	body_def.position = b2Vec2{ transform.position.x, transform.position.y };
	body_def.rotation = quat_to_b2rot( transform.rotation );

	// Store entity ID in userData by casting it to a pointer
	body_def.userData = reinterpret_cast<void*>( static_cast<uintptr_t>( entity ) );

	auto physics_body = mPhysicsEngine->CreateBody( body_def );

	// Add circle collider
	if( mECSRegistry.HasComponent<CircleColliderComponent>( entity ) )
	{
		auto& circle_collider = mECSRegistry.GetComponent<CircleColliderComponent>( entity );
		mPhysicsEngine->AddCircleColliderToBody( *physics_body, circle_collider.radius, circle_collider.offset );
	}

	// Add box collider
	if( mECSRegistry.HasComponent<BoxColliderComponent>( entity ) )
	{
		auto& box_collider = mECSRegistry.GetComponent<BoxColliderComponent>( entity );
		mPhysicsEngine->AddBoxColliderToBody( *physics_body, box_collider.width, box_collider.height, box_collider.offset );
	}

	// @todo more collider types

	// Store the body in the resource manager and save the ID in the component
	collision.body_id = mBodyManager.Store( std::move( physics_body ) );
}

void
PhysicsSystem::SyncTransformToStaticBodies( uint64_t entity )
{
	auto& collision = mECSRegistry.GetComponent<CollisionComponent>( entity );
	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );

	if( !collision.is_static || collision.body_id == INVALID_ID || !transform.dirty )
	{
		return; // Only sync static bodies with valid body IDs
	}

	auto body = mBodyManager.Get( collision.body_id );
	if( !body )
	{
		LOG_ERROR() << "Invalid body ID in CollisionComponent for entity " << entity;
		return; // Invalid body ID
	}

	mPhysicsEngine->UpdateBodyTransform( *body, transform.position, quat_to_b2rot( transform.rotation ) );
}
