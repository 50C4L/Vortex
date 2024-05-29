#include "Player.h"

#include <graphics/RenderComponent.h>
#include <graphics/Renderer.h>
#include <audio/AudioMixer.h>

#include "Ship.h"
#include "GameConfig.h"

using namespace vortex;
using namespace vortex::config;

namespace
{
	// speeds - per second
	const float ROTATION_SPEED = 200.0f;
	const float THRUST_ACCELERATION = 100.0f;
	const float MAX_THRUST_SPEED = 200.f;
}

Player::Player( graphics::Renderer& renderer, events::InputController& input_controller )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mShip( std::make_unique<Ship>( renderer ) )
{
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_THRUST ), this );
}

Player::~Player()
{
}

void
Player::Init( SingleTextureSpriteMaterial& material,
			  SingleTextureSpriteMaterial::Resources& resources,
			  std::unique_ptr<audio::SoundInstance> engine_sound )
{
	mShip->SetBodyMaterial( material.Instantiate( mRenderer, resources ) );
	mShip->SetThrustMaterial( material.Instantiate( mRenderer, resources ) );

	mShip->SetThrustAcceleration( THRUST_ACCELERATION );
	mShip->SetMaxThrustSpeed( MAX_THRUST_SPEED );

	mEngineSound = std::move( engine_sound );
}

void
Player::Update()
{
	std::chrono::time_point<std::chrono::steady_clock> current_time = std::chrono::steady_clock::now();
	std::chrono::duration<float, std::milli> delta_time_ms = current_time - mLastUpdateTime;
	mLastUpdateTime = current_time;

	if( mRotateState.left )
	{
		mShip->SetRotateSpeed( ROTATION_SPEED );
	}
	else if( mRotateState.right )
	{
		mShip->SetRotateSpeed( -1.f * ROTATION_SPEED );
	}
	else
	{
		mShip->SetRotateSpeed( 0.f );
	}

	mShip->Update( delta_time_ms.count());
}

void
Player::Draw()
{
	mShip->Draw();
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
	case GameEvents::PLAYER_THRUST:
		mShip->Thrust( on );
		if( on )
		{
			mEngineSound->Play();
		}
		else
		{
			mEngineSound->Stop();
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