#include "Ship.h"

#include <assets/TextureAtlas.h>

#include <graphics/BuiltInMeshes.h>
#include <graphics/Renderer.h>
#include <graphics/RenderComponent.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>

#include <utility/Logger.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace vortex;
using namespace utility;

Ship::Ship( eage::graphics::Renderer& renderer )
	: mRenderer( renderer )
	, mRotateSpeed( 0.f )
	, mMaxThrustSpeed( 0.f )
	, mThrustAcceleration( 0.f )
	, mVelocity( { 0.f } )
	, mForwardDir( { 0.f, 1.f, 0.f } )
	, mPosition( { 0.f } )
{
	for( size_t i = 0; i < 2; ++i )
	{
		mRenderComponents.push_back( std::make_unique<eage::graphics::RenderComponent>( mRenderer ) );
	}
	mShipBodyRCIndex = 0;
	mThrustRCIndex = 1;

	assets::TextureAtlas texture_atlas( "./resources/textures/ship/ship_texatlas.json" );
	texture_atlas.Flip();
	const auto& ship_tex = texture_atlas.GetSubTexture( "player_ship.png" );

	// Create the ship body
	auto rect = eage::graphics::made_rect_vertices( { 0, 0, 0 }, 50, 50 );

	rect.vertices[0].uv_x = ship_tex.uv_max.x;
	rect.vertices[0].uv_y = ship_tex.uv_min.y;
	rect.vertices[1].uv_x = ship_tex.uv_max.x;
	rect.vertices[1].uv_y = ship_tex.uv_max.y;
	rect.vertices[2].uv_x = ship_tex.uv_min.x;
	rect.vertices[2].uv_y = ship_tex.uv_min.y;
	rect.vertices[3].uv_x = ship_tex.uv_min.x;
	rect.vertices[3].uv_y = ship_tex.uv_max.y;

	mRenderComponents[mShipBodyRCIndex]->SetMeshBuffer( std::move( mRenderer.UploadMesh( rect.indices, rect.vertices ) ), 0, 6, 0 );

	// Create the thrust
	const auto& thrust_tex = texture_atlas.GetSubTexture( "ship_thrust_fx.png" );
	rect = eage::graphics::made_rect_vertices( { 0, -30.f, 0 }, 10, 10 );
	rect.vertices[0].uv_x = thrust_tex.uv_max.x;
	rect.vertices[0].uv_y = thrust_tex.uv_min.y;
	rect.vertices[1].uv_x = thrust_tex.uv_max.x;
	rect.vertices[1].uv_y = thrust_tex.uv_max.y;
	rect.vertices[2].uv_x = thrust_tex.uv_min.x;
	rect.vertices[2].uv_y = thrust_tex.uv_min.y;
	rect.vertices[3].uv_x = thrust_tex.uv_min.x;
	rect.vertices[3].uv_y = thrust_tex.uv_max.y;

	mRenderComponents[mThrustRCIndex]->SetMeshBuffer( mRenderer.UploadMesh( rect.indices, rect.vertices ), 0, 6, 0 );
}

Ship::~Ship()
{
}

void
Ship::SetBodyMaterial( std::unique_ptr<eage::graphics::Material> material )
{
	mRenderComponents[mShipBodyRCIndex]->SetMaterial( std::move( material ) );
}

void
Ship::SetThrustMaterial( std::unique_ptr<eage::graphics::Material> material )
{
	mRenderComponents[mThrustRCIndex]->SetMaterial( std::move( material ) );
}

void
Ship::Update( glm::mat4 transform )
{
	mRenderComponents[mShipBodyRCIndex]->Transform( transform );
	mRenderComponents[mThrustRCIndex]->Transform( transform );
}

void
Ship::Draw( bool is_thrust_on )
{
	if( is_thrust_on )
	{
		mRenderer.AddToRenderQueue( mRenderComponents[mThrustRCIndex]->CreateRenderInfo() );
	}

	mRenderer.AddToRenderQueue( mRenderComponents[mShipBodyRCIndex]->CreateRenderInfo() );
}
