#include "PhysicsEngine.h"

#include <utility/Logger.h>

using namespace eage::physics;
using namespace utility;

PhysicsBody::PhysicsBody( b2BodyId body_id )
	: mBodyId( body_id )
{
}

PhysicsBody::~PhysicsBody()
{
	b2DestroyBody( mBodyId );
}

PhysicsEngine::PhysicsEngine()
	: mWorldId( b2_nullWorldId )
	, mLastUpdateTime( std::chrono::steady_clock::now() )
{
}

PhysicsEngine::~PhysicsEngine()
{
	if( B2_IS_NON_NULL( mWorldId ) )
	{
		b2DestroyWorld( mWorldId );
		mWorldId = b2_nullWorldId;
	}
}

void
PhysicsEngine::CreateWorld( glm::vec2 gravity )
{
	// Initialize the physics world here
	b2WorldDef world_def = b2DefaultWorldDef();
	world_def.gravity = b2Vec2{ gravity.x, gravity.y };
	mWorldId = b2CreateWorld( &world_def );

	LOG() << "Physics world created with gravity: (" << gravity.x << ", " << gravity.y << ")";
}

std::unique_ptr<PhysicsBody>
PhysicsEngine::CreateBody( const b2BodyDef& body_def )
{
	if( B2_IS_NULL( mWorldId ) )
	{
		LOG_ERROR() << "Cannot create body: Physics world is not initialized.";
		return std::make_unique<PhysicsBody>( b2_nullBodyId );
	}
	return std::make_unique<PhysicsBody>( b2CreateBody( mWorldId, &body_def ) );
}

void
PhysicsEngine::AddCircleColliderToBody( PhysicsBody& body, float radius, glm::vec2 offset )
{
	b2Circle circle;
	circle.center = b2Vec2{ offset.x, offset.y };
	circle.radius = radius;
	b2ShapeDef def = b2DefaultShapeDef();
	b2CreateCircleShape( body.mBodyId, &def, &circle );
}

void 
PhysicsEngine::AddBoxColliderToBody( PhysicsBody& body, float width, float height, glm::vec2 offset )
{
	b2Polygon box;
	if( offset == glm::vec2(0.0f, 0.0f) )
	{
		box = b2MakeBox( width, height );
	}
	else
	{
		box = b2MakeOffsetBox( width, height, b2Vec2{ offset.x, offset.y }, 0.0f );
	}
	
	b2ShapeDef def = b2DefaultShapeDef();
	b2CreatePolygonShape( body.mBodyId, &def, &box );
}

void
PhysicsEngine::UpdateBodyTransform( PhysicsBody& body, glm::vec2 position, b2Rot rotation )
{
	b2Body_SetTransform( body.mBodyId, b2Vec2{ position.x, position.y }, rotation );
}

void
PhysicsEngine::Update()
{
	if( B2_IS_NULL( mWorldId ) )
	{
		LOG_ERROR() << "Cannot update physics: Physics world is not initialized.";
		return;
	}

	// Only step the world if the fixed timestep has elapsed
	if( std::chrono::steady_clock::now() - mLastUpdateTime < std::chrono::milliseconds(16) )
	{
		return;
	}

	b2World_Step( mWorldId, 1.f / 60.f, 0 );
	mLastUpdateTime = std::chrono::steady_clock::now();
}