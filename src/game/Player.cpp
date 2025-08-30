#include "Player.h"

#include <graphics/BuiltInMeshes.h>
#include <graphics/Renderer.h>
#include <graphics/BuiltInUniforms.h>

#include <audio/AudioMixer.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <assets/TextureAtlas.h>

#include "GameConfig.h"
#include "components/ShipStateComponents.h"

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

Player::Player( eage::graphics::Renderer& renderer, events::InputController& input_controller, ECSRegistry& ecs_registry, 
				RenderSystem& render_system )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mEcsRegistry( ecs_registry )
	, mRenderSystem( render_system )
{
	// Init ship componments
	mShipEntity = mEcsRegistry.CreateEntity();
	mEcsRegistry.AddComponent<eage::ecs::TransformComponent>( mShipEntity, TransformComponent{ { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f } } );
	mEcsRegistry.AddComponent<eage::ecs::Velocity2DComponent>( mShipEntity, eage::ecs::Velocity2DComponent{ { 0.f, 0.f, 0.f } } );
	mEcsRegistry.AddComponent<components::ShipStateComponent>( mShipEntity, components::ShipStateComponent{ false } );

	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_LEFT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_ROTATE_RIGHT ), this );
	mInputController.Subscribe( static_cast<uint64_t>( GameEvents::PLAYER_THRUST ), this );
}

Player::~Player()
{
}

void
Player::Init( eage::ecs::ResourceId sprite_material_id,
			  std::unique_ptr<audio::SoundInstance> engine_sound )
{
	assets::TextureAtlas texture_atlas( "./resources/textures/ship/ship_texatlas.json" );
	texture_atlas.Flip();
	const auto& ship_tex = texture_atlas.GetSubTexture( "player_ship.png" ); 

	auto rect = eage::graphics::made_rect_vertices( { 0, 0, 0 }, 50, 50 );
	rect.vertices[0].uv_x = ship_tex.uv_max.x;
	rect.vertices[0].uv_y = ship_tex.uv_min.y;
	rect.vertices[1].uv_x = ship_tex.uv_max.x;
	rect.vertices[1].uv_y = ship_tex.uv_max.y;
	rect.vertices[2].uv_x = ship_tex.uv_min.x;
	rect.vertices[2].uv_y = ship_tex.uv_min.y;
	rect.vertices[3].uv_x = ship_tex.uv_min.x;
	rect.vertices[3].uv_y = ship_tex.uv_max.y;

	// Create ship mesh through RenderSystem
	auto ship_mesh_id = mRenderSystem.CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );

	// Create uniform buffer and descriptor for mesh data
	auto ship_uniform_buffer_id = mRenderSystem.CreateDynamicUniformBuffer( sizeof(eage::graphics::MeshUniformData) );
	auto ship_descriptor_id = mRenderSystem.CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
	
	// Set up the descriptor binding for the ship (this should be done once at creation time)
	mRenderSystem.GetDescriptorSet( ship_descriptor_id )->WriteBuffer(
		0, // binding
		vk::DescriptorType::eUniformBufferDynamic,
		mRenderSystem.GetUniformBuffer( ship_uniform_buffer_id )->buffer,
		sizeof(eage::graphics::MeshUniformData) );

	// Create the thrust mesh
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

	// auto thrust_mesh_id = mRenderSystem.CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );
	// auto thrust_uniform_buffer_id = mRenderSystem.CreateDynamicUniformBuffer( sizeof(eage::graphics::MeshUniformData) );
	// auto thrust_descriptor_id = mRenderSystem.CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
	
	// Set up the descriptor binding for the thrust effect
	// mRenderSystem.GetDescriptorSet( thrust_descriptor_id )->WriteBuffer(
	// 	0, // binding
	// 	vk::DescriptorType::eUniformBufferDynamic,
	// 	mRenderSystem.GetUniformBuffer( thrust_uniform_buffer_id )->buffer,
	// 	sizeof(eage::graphics::MeshUniformData) );

	// Add ECS RenderComponent with the new resource IDs
	mEcsRegistry.AddComponent<RenderComponent>( mShipEntity, RenderComponent{ 
		ship_mesh_id, 
		sprite_material_id, 
		ship_uniform_buffer_id, 
		ship_descriptor_id 
	} );

	mEngineSound = std::move( engine_sound );
}

void
Player::Update()
{
	// std::chrono::time_point<std::chrono::steady_clock> current_time = std::chrono::steady_clock::now();
	// std::chrono::duration<float, std::milli> delta_time_ms = current_time - mLastUpdateTime;
	// mLastUpdateTime = current_time;

	// if( mRotateState.left )
	// {
	// 	mShipControlSystem->Rotate( ROTATION_SPEED );
	// }
	// else if( mRotateState.right )
	// {
	// 	mShipControlSystem->Rotate( -1.f * ROTATION_SPEED );
	// }
	// else
	// {
	// 	mShipControlSystem->Rotate( 0.f );
	// }

	// mShipControlSystem->Update( delta_time_ms.count() );
}

void
Player::Draw()
{
	// Drawing is now handled by the ECS RenderSystem
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