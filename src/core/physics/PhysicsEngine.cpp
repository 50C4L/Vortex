#include "PhysicsEngine.h"

#include <physics/PhysicsEventListener.h>
#include <utility/Logger.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace eage::physics;
using namespace utility;

// PhysicsBody implementation
PhysicsBody::PhysicsBody( b2BodyId body_id )
	: mBodyId( body_id )
{
}

PhysicsBody::~PhysicsBody()
{
	b2DestroyBody( mBodyId );
}

// PhysicsEngine implementation
PhysicsEngine::PhysicsEngine()
	: mWorldId( b2_nullWorldId )
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

	mDebugDraw = {};
	mDebugDraw.drawShapes = true;
	mDebugDraw.DrawPolygon =
		[]( const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context )
		{
			// LOG() << "Physic shape polygon debug info: ";
			// for( int i = 0; i < vertexCount; ++i )
			// {
			// 	LOG() << "  Vertex " << i << ": (" << vertices[i].x << ", " << vertices[i].y << ")";
			// }
		};
	mDebugDraw.DrawSolidPolygon =
		[]( b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context )
		{
			// LOG() << "Physic shape solid polygon debug info at position: (" 
			// 	  << transform.p.x << ", " << transform.p.y << ") rotation: " 
			// 	  << b2Rot_GetAngle(transform.q) << " rad";
			// for( int i = 0; i < vertexCount; ++i )
			// {
			// 	LOG() << "  Vertex " << i << ": (" << vertices[i].x << ", " << vertices[i].y << ")";
			// }
		};
	mDebugDraw.DrawCircle =
		[]( b2Vec2 center, float radius, b2HexColor color, void* context )
		{
			// LOG() << "Physic shape circle debug info: Center(" << center.x << ", " << center.y << "), Radius: " << radius;
		};
	mDebugDraw.DrawSolidCircle =
		[]( b2Transform transform, float radius, b2HexColor color, void* context )
		{
			// LOG() << "Physic shape solid circle debug info at position: (" 
			// 	  << transform.p.x << ", " << transform.p.y << ") rotation: " 
			// 	  << b2Rot_GetAngle(transform.q) << " rad, Radius: " << radius;
		};
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
PhysicsEngine::AddCircleColliderToBody( PhysicsBody& body, CollisionFilter filter, float radius, bool is_sensor, glm::vec2 offset )
{
	b2Circle circle;
	circle.center = b2Vec2{ offset.x, offset.y };
	circle.radius = radius;

	b2ShapeDef def = b2DefaultShapeDef();
	def.isSensor = is_sensor;
	def.enableSensorEvents = true;
	def.enableContactEvents = !is_sensor;
	def.filter.categoryBits = filter.category_bits;
	def.filter.maskBits = filter.mask_bits;
	def.filter.groupIndex = filter.group_index;
	def.userData = &body;
	b2CreateCircleShape( body.mBodyId, &def, &circle );
}

void 
PhysicsEngine::AddBoxColliderToBody( PhysicsBody& body, CollisionFilter filter, float width, float height, bool is_sensor, glm::vec2 offset )
{
	b2Polygon box;
	if( offset == glm::vec2(0.0f, 0.0f) )
	{
		box = b2MakeBox( width / 2.f, height / 2.f );
	}
	else
	{
		box = b2MakeOffsetBox( width / 2.f, height / 2.f, b2Vec2{ offset.x, offset.y }, 0.0f );
	}
	
	b2ShapeDef def = b2DefaultShapeDef();
	def.isSensor = is_sensor;
	def.enableSensorEvents = true;
	def.enableContactEvents = !is_sensor;
	def.filter.categoryBits = filter.category_bits;
	def.filter.maskBits = filter.mask_bits;
	def.filter.groupIndex = filter.group_index;
	def.userData = &body;
	b2CreatePolygonShape( body.mBodyId, &def, &box );
}

void
PhysicsEngine::UpdateBodyTransform( PhysicsBody& body, glm::vec2 position, b2Rot rotation )
{
	b2Body_SetTransform( body.mBodyId, b2Vec2{ position.x, position.y }, rotation );
}

void
PhysicsEngine::Update( float dt )
{
	if( B2_IS_NULL( mWorldId ) )
	{
		LOG_ERROR() << "Cannot update physics: Physics world is not initialized.";
		return;
	}

	constexpr float FIXED_STEP = 1.f / 60.f;
	mAccumulator += dt;
	while( mAccumulator >= FIXED_STEP )
	{
		b2World_Step( mWorldId, FIXED_STEP, 4 );
		mAccumulator -= FIXED_STEP;
	}

	ProcessSensorEvents();
	ProcessContactEvents();

	b2World_Draw( mWorldId, &mDebugDraw );
}

PhysicsEngine::PhysicsBodyTransform
PhysicsEngine::GetBodyTransform( const PhysicsBody& body )
{
	if( B2_IS_NULL( body.mBodyId ) )
	{
		LOG_ERROR() << "Cannot get body transform: Body is not valid.";
		return {};
	}

	b2Transform transform = b2Body_GetTransform( body.mBodyId );
	glm::vec3 position = glm::vec3( transform.p.x, transform.p.y, 0.0f );
	float angle = b2Rot_GetAngle( transform.q );
	glm::quat rotation = glm::angleAxis( angle, glm::vec3( 0.0f, 0.0f, 1.0f ) );
	return { std::move( position ), std::move( rotation ) };
}

void
PhysicsEngine::SetEventListener( PhysicsEventListener* listener )
{
	mEventListener = listener;
}

void
PhysicsEngine::ClearEventListener()
{
	mEventListener = nullptr;
}

void*
PhysicsEngine::GetUserData( const PhysicsBody& body ) const
{
	if( B2_IS_NULL( body.mBodyId ) )
	{
		LOG_ERROR() << "Cannot get body user data: Body is not valid.";
		return nullptr;
	}

	return b2Body_GetUserData( body.mBodyId );
}

void
PhysicsEngine::ProcessSensorEvents()
{
	b2SensorEvents sensor_events = b2World_GetSensorEvents( mWorldId );

	if( mEventListener )
	{
		// Process begin touch events
		for( int i = 0; i < sensor_events.beginCount; ++i )
		{
			b2SensorBeginTouchEvent event = sensor_events.beginEvents[i];
			PhysicsBody* sensor_body = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.sensorShapeId ) );
			PhysicsBody* visitor_body = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.visitorShapeId ) );
			if( sensor_body && visitor_body )
			{
				mEventListener->OnSensorEnter( sensor_body, visitor_body );
			}
		}

		// Process end touch events
		for( int i = 0; i < sensor_events.endCount; ++i )
		{
			b2SensorEndTouchEvent event = sensor_events.endEvents[i];
			PhysicsBody* sensor_body = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.sensorShapeId ) );
			PhysicsBody* visitor_body = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.visitorShapeId ) );
			if( sensor_body && visitor_body )
			{
				mEventListener->OnSensorExit( sensor_body, visitor_body );
			}
		}
	}
}

void
PhysicsEngine::ProcessContactEvents()
{
	b2ContactEvents contact_events = b2World_GetContactEvents( mWorldId );

	if( mEventListener )
	{
		// Process begin touch events
		for( int i = 0; i < contact_events.beginCount; ++i )
		{
			b2ContactBeginTouchEvent event = contact_events.beginEvents[i];
			PhysicsBody* body_a = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.shapeIdA ) );
			PhysicsBody* body_b = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.shapeIdB ) );
			if( body_a && body_b )
			{
				mEventListener->OnCollideBegin( body_a, body_b );
			}
		}

		// Process end touch events
		for( int i = 0; i < contact_events.endCount; ++i )
		{
			b2ContactEndTouchEvent event = contact_events.endEvents[i];
			PhysicsBody* body_a = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.shapeIdA ) );
			PhysicsBody* body_b = static_cast<PhysicsBody*>( b2Shape_GetUserData( event.shapeIdB ) );
			if( body_a && body_b )
			{
				mEventListener->OnCollideEnd( body_a, body_b );
			}
		}
	}
}
