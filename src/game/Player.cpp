#include "Player.h"

#include <graphics/RenderComponent.h>
#include <graphics/Renderer.h>
#include <audio/AudioMixer.h>
#include <ecs/components/Basics.h>

#include "Ship.h"
#include "GameConfig.h"
#include "components/ShipStateComponents.h"
#include "systems/ShipControlSystem.h"

using namespace vortex;
using namespace vortex::config;
using namespace eage::ecs;

namespace
{
	// speeds - per second
	const float ROTATION_SPEED = 200.0f;
	const float THRUST_ACCELERATION = 100.0f;
	const float MAX_THRUST_SPEED = 200.f;
}

Player::Player( graphics::Renderer& renderer, events::InputController& input_controller, ECSRegistry& ecs_registry )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mEcsRegistry( ecs_registry )
	, mShip( std::make_unique<Ship>( renderer ) )
{
	// Init ship componments
	mShipEntity = mEcsRegistry.CreateEntity();
	mEcsRegistry.AddComponent<eage::ecs::TransformComponent>( mShipEntity, TransformComponent{ { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f } } );
	mEcsRegistry.AddComponent<eage::ecs::VelocityComponent>( mShipEntity, eage::ecs::VelocityComponent{ { 0.f, 0.f, 0.f } } );
	mEcsRegistry.AddComponent<components::ShipStateComponent>( mShipEntity, components::ShipStateComponent{ false } );

	// Init ship coontrol system
	mShipControlSystem = std::make_unique<ShipControlSystem>( mEcsRegistry, mShipEntity );

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
	// @todo: Is this a good way to pass in resources?
	mShip->SetBodyMaterial( material.Instantiate( mRenderer, resources ) );
	mShip->SetThrustMaterial( material.Instantiate( mRenderer, resources ) );

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
		mShipControlSystem->Rotate( ROTATION_SPEED );
	}
	else if( mRotateState.right )
	{
		mShipControlSystem->Rotate( -1.f * ROTATION_SPEED );
	}
	else
	{
		mShipControlSystem->Rotate( 0.f );
	}

	mShipControlSystem->Update( delta_time_ms.count() );
	auto& transform = mEcsRegistry.GetComponent<eage::ecs::TransformComponent>( mShipEntity );
	mShip->Update( transform.ToMatrix() );
}

void
Player::Draw()
{
	auto& ship_state = mEcsRegistry.GetComponent<components::ShipStateComponent>( mShipEntity );
	mShip->Draw( ship_state.is_thrust_on );
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
		if( on )
		{
			mEngineSound->Play();
		}
		else
		{
			mEngineSound->Stop();
		}

		{
			
			mShipControlSystem->Thrust( on );
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