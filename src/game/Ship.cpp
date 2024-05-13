#include "Ship.h"

#include <graphics/Renderer.h>
#include <graphics/RenderComponent.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>

using namespace vortex;

Ship::Ship( graphics::Renderer& renderer )
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
	mRenderComponent->Update();
}

void
Ship::Rotate( float angle )
{
	mRenderComponent->Rotate( angle, glm::vec3( 0.0f, 0.0f, 1.0f ) );
}