#include "Player.h"

#include <graphics/RenderComponent.h>
#include <graphics/Renderer.h>

#include "Ship.h"

using namespace vortex;

Player::Player( graphics::Renderer& renderer )
	: mRenderer( renderer )
	, mShip( std::make_unique<Ship>( renderer ) )
{
}

Player::~Player()
{
}

void
Player::Init( std::shared_ptr<graphics::GPUMeshBuffers> mesh_buffer, std::shared_ptr<graphics::Material> material )
{
	mShip->GetRenderComponent()->SetMeshBuffer( std::move( mesh_buffer ), 0, 6, 0 );
	mShip->GetRenderComponent()->SetMaterial( std::move( material ) );
}

void
Player::Update()
{
	mShip->Update( 0.0f );
}

void
Player::Draw()
{
	mRenderer.AddToRenderQueue( mShip->GetRenderComponent().get() );
}