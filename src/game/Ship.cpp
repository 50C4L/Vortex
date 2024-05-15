#include "Ship.h"

#include <graphics/Renderer.h>
#include <graphics/RenderComponent.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>

#include <utility/Logger.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace vortex;
using namespace utility;

Ship::Ship( graphics::Renderer& renderer )
	: mRotateSpeed( 0.f )
	, mMaxThrustSpeed( 0.f )
	, mThrustAcceleration( 0.f )
	, mVelocity( { 0.f } )
	, mForwardDir( { 0.f, 1.f, 0.f } )
	, mIsThrustOn( false )
	, mPosition( { 0.f } )
{
	// Create the render component
	mRenderComponent = std::make_shared<graphics::RenderComponent>( renderer );
}

Ship::~Ship()
{
}

std::shared_ptr<graphics::RenderComponent>
Ship::GetRenderComponent() const
{
	return mRenderComponent;
}

void
Ship::Update( float delta_time )
{
	if( mIsThrustOn )
	{
		mVelocity += mForwardDir * ( mThrustAcceleration * ( delta_time / 1000.0f ) );
		if( glm::length( mVelocity ) > mMaxThrustSpeed )
		{
			mVelocity = mForwardDir * mMaxThrustSpeed;
		}
	}

	auto new_pos = mPosition + mVelocity * ( delta_time / 1000.0f );
	mRenderComponent->Translate( new_pos - mPosition );
	mPosition = new_pos;

	if( mRotateSpeed != 0.f )
	{
		float rotation = mRotateSpeed / 1000.f * delta_time;
		auto rotate_mat = mRenderComponent->Rotate( rotation, glm::vec3( 0.0f, 0.0f, 1.0f ), true );
		rotate_mat = glm::inverse( rotate_mat );
		mForwardDir = glm::vec3( glm::vec4( mForwardDir, 0.0f ) * rotate_mat);
	}
	
	mRenderComponent->Update();
}

void
Ship::SetRotateSpeed( float angle )
{
	mRotateSpeed = angle;
}

void
Ship::SetMaxThrustSpeed( float speed )
{
	mMaxThrustSpeed = speed;
}

void
Ship::SetThrustAcceleration( float acceleration )
{
	mThrustAcceleration = acceleration;
}

void
Ship::Thrust( bool on )
{
	mIsThrustOn = on;
}