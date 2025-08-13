#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace eage::graphics;

OrthographicCamera::OrthographicCamera( float left, float right, float bottom, float top, float near, float far )
	: mPosition( 0.0f, 0.0f, 1.0f )
	, mViewMatrix( 1.0f )
	, mProjectionMatrix( 1.0f )
{
	mProjectionMatrix = glm::ortho( left, right, bottom, top, near, far );
	mProjectionMatrix[1][1] *= -1;
	mViewMatrix = glm::lookAt( mPosition, glm::vec3{ 0.f }, glm::vec3{ 0.f, 1.f, 0.f } );
}

void
OrthographicCamera::SetPosition( const glm::vec3& position )
{
	mPosition = position;
	mViewMatrix = glm::translate( glm::mat4( 1.0f ), -mPosition );
}

const glm::vec3&
OrthographicCamera::GetPosition() const
{
	return mPosition;
}

const glm::mat4&
OrthographicCamera::GetViewMatrix() const
{
	return mViewMatrix;
}

const glm::mat4&
OrthographicCamera::GetProjectionMatrix() const
{
	return mProjectionMatrix;
}