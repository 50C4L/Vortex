#include "Player.h"

#include <graphics/RenderComponent.h>
#include <graphics/Renderer.h>

#include "Ship.h"
#include "GameConfig.h"

using namespace vortex;
using namespace vortex::config;

Player::Player( graphics::Renderer& renderer, events::InputController& input_controller )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mShip( std::make_unique<Ship>( renderer ) )
{
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ), this );
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
	if( mRotateState.left )
	{
		mShip->Rotate( 1.f );
	}
	else if( mRotateState.right )
	{
		mShip->Rotate( -1.f );
	}

	mShip->Update( 0.0f );
}

void
Player::Draw()
{
	mRenderer.AddToRenderQueue( mShip->GetRenderComponent().get() );
}

void
Player::OnInputEvent( uint64_t event_id, bool on )
{
	auto event = static_cast<GameEvents>( event_id );
	switch( event )
	{
	case GameEvents::PLAYER_ROTATE_LEFT:
		if( mRotateState.right )
		{
			StopRotation();
			break;
		}

		if( mRotateState.left != on )
		{
			mRotateState.left = on;
		}
		break;
	case GameEvents::PLAYER_ROTATE_RIGHT:
		if( mRotateState.left )
		{
			StopRotation();
			break;
		}

		if( mRotateState.right != on )
		{
			mRotateState.right = on;
		}
		break;
	default:
		break;
	}
}

void
Player::StopRotation()
{
	mRotateState.left = false;
	mRotateState.right = false;
}