#include "PlayerGameplaySystem.h"

#include <ecs/components/Audio.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>
#include <ecs/ResourceManager.h>
#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <assets/TextureAtlas.h>
#include <graphics/MaterialBuilder.h>
#include <utility/Logger.h>

#include "BulletSystem.h"
#include "../GameConfig.h"
#include "../components/GameGenericComponents.h"
#include "../components/HealthComponent.h"
#include "../components/PlayerComponents.h"


using namespace vortex;
using namespace vortex::config;
using namespace utility;


PlayerGameplaySystem::PlayerGameplaySystem( eage::ecs::ECSRegistry& registry, BulletSystem& bullet_system,
											eage::ecs::RenderSystem& render_system, eage::ecs::AudioSystem& audio_system )
	: mRegistry( registry )
	, mBulletSystem( bullet_system )
	, mRenderSystem( render_system )
	, mAudioSystem( audio_system )
{
}

void
PlayerGameplaySystem::PreparePlayer( uint64_t root_entity )
{
	// ------------------------------------------------------------------
	// Material
	// ------------------------------------------------------------------
	mRenderSystem.CreateImageBuffer( "./resources/textures/ship/ship_texatlas.png" );
	mRenderSystem.CreateImageBuffer( "./resources/textures/bullets/player_bullets.png" );

	auto ship_material_prop = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.AddTexture( "./resources/textures/ship/ship_texatlas.png",
					 eage::graphics::TextureFilter::NEAREST, eage::graphics::TextureFilter::NEAREST )
		.SetAlphaBlending()
		.EnableDepthTest( true )
		.Build();

	mPlayerMaterialId = mRenderSystem.CreateMaterial( ship_material_prop );

	auto bullet_material_prop = eage::graphics::MaterialBuilder()
		.SetShaders( "./src/shaders/compiled/colored_triangle_mesh.vert.spv",
					 "./src/shaders/compiled/colored_triangle.frag.spv" )
		.AddTexture( "./resources/textures/bullets/player_bullets.png",
					 eage::graphics::TextureFilter::NEAREST, eage::graphics::TextureFilter::NEAREST )
		.SetAlphaBlending()
		.EnableDepthTest( true )
		.Build();

	mPlayerBulletMaterialId = mRenderSystem.CreateMaterial( bullet_material_prop );

	assets::TextureAtlas texture_atlas( "./resources/textures/ship/ship_texatlas.json" );
	texture_atlas.Flip();
	assets::TextureAtlas bullet_texture_atlas( "./resources/textures/bullets/player_bullets.json" );
	bullet_texture_atlas.Flip();

	// ------------------------------------------------------------------
	// Player entity
	// ------------------------------------------------------------------
	auto player_entity = mRegistry.CreateEntity();

	auto& root = mRegistry.GetComponent<eage::ecs::SceneGraphComponment>( root_entity );
	root.children_entities.push_back( player_entity );
	eage::ecs::SceneGraphComponment player_relationship;
	player_relationship.parent_entity = root_entity;
	mRegistry.AddComponent( player_entity, std::move( player_relationship ) );

	PlayerComponent player;
	mRegistry.AddComponent( player_entity, std::move( player ) );

	mRegistry.AddComponent( player_entity, HealthComponent{} );

	mRegistry.AddComponent( player_entity, eage::ecs::TransformComponent{} );

	eage::ecs::PhysicsComponent player_physics;
	player_physics.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
	player_physics.sync_transform_from_body = true;
	player_physics.max_linear_velocity = 400.0f;
	mRegistry.AddComponent( player_entity, std::move( player_physics ) );

	eage::ecs::CircleColliderComponent player_collider;
	player_collider.radius = 25.f;
	player_collider.category_bits = PHYSX_CAT_WARPABLE | PHYSX_CAT_PLAYER;
	player_collider.mask_bits = PHYSX_CAT_SCREEN_ZONE | PHYSX_CAT_ENEMY;
	mRegistry.AddComponent( player_entity, std::move( player_collider ) );

	const auto& ship_tex = texture_atlas.GetSubTexture( "Ship.png" );
	mRenderSystem.AttachSprite( player_entity, mPlayerMaterialId, 50.f, 50.f, ship_tex.uv_min, ship_tex.uv_max );

	AudioSourceComponent thrust_audio;
	thrust_audio.sources["thruster"] = { mAudioSystem.LoadSound( { "./resources/sounds/thruster.mp3", 1, true } ) };
	mRegistry.AddComponent( player_entity, std::move( thrust_audio ) );
	mRegistry.AddComponent( player_entity, AudioEventComponent{} );

	mRegistry.AddComponent( player_entity, WarpComponent{} );

	// ------------------------------------------------------------------
	// Thruster child entity
	// ------------------------------------------------------------------
	auto thruster_entity = mRegistry.CreateEntity();

	auto& player_scene = mRegistry.GetComponent<eage::ecs::SceneGraphComponment>( player_entity );
	player_scene.children_entities.push_back( thruster_entity );

	auto& player_cmp = mRegistry.GetComponent<PlayerComponent>( player_entity );
	player_cmp.thruster_fx_entity = thruster_entity;

	eage::ecs::TransformComponent thruster_transform;
	thruster_transform.SetPosition( glm::vec3( 0.0f, -30.f, 0.0f ) );
	thruster_transform.SetScale( glm::vec3( 1 / 5.f ) );
	mRegistry.AddComponent( thruster_entity, std::move( thruster_transform ) );

	const auto& thrust_tex = texture_atlas.GetSubTexture( "ship_thrust_fx.png" );
	mRenderSystem.AttachSprite( thruster_entity, mPlayerMaterialId, 50.f, 50.f, thrust_tex.uv_min, thrust_tex.uv_max );

	// ------------------------------------------------------------------
	// Bullet launcher child entity
	// ------------------------------------------------------------------
	auto launcher_entity = mRegistry.CreateEntity();
	player_scene.children_entities.push_back( launcher_entity );
	player_cmp.bullet_launcher_entity = launcher_entity;

	eage::ecs::SceneGraphComponment launcher_relationship;
	launcher_relationship.parent_entity = player_entity;
	mRegistry.AddComponent( launcher_entity, std::move( launcher_relationship ) );

	eage::ecs::TransformComponent launcher_transform;
	launcher_transform.SetPosition( glm::vec3( 0.f, 25.f, 0.f ) );
	mRegistry.AddComponent( launcher_entity, std::move( launcher_transform ) );

	AudioSourceComponent launcher_audio;
	launcher_audio.sources["fire"] = { mAudioSystem.LoadSound( { "./resources/sounds/laser.wav", 4, false } ) };
	mRegistry.AddComponent( launcher_entity, std::move( launcher_audio ) );
	mRegistry.AddComponent( launcher_entity, AudioEventComponent{} );

	// ------------------------------------------------------------------
	// Bullet pool
	// ------------------------------------------------------------------
	const auto& default_bullet_tex = bullet_texture_atlas.GetSubTexture( "p_default_bullet.png" );

	BulletPoolConfig bullet_config;
	bullet_config.damage = 1.f;
	bullet_config.collider_radius = 5.f;
	bullet_config.mesh_width = 10.f;
	bullet_config.mesh_height = 10.f;
	bullet_config.material_id = mPlayerBulletMaterialId;
	bullet_config.category_bits = PHYSX_CAT_BULLET;
	bullet_config.mask_bits = PHYSX_CAT_ENEMY;
	bullet_config.uv_min = default_bullet_tex.uv_min;
	bullet_config.uv_max = default_bullet_tex.uv_max;
	bullet_config.fire_interval = 0.5f; // 2 bullets per second max
	mDefaultBulletPoolId = mBulletSystem.PreparePool( bullet_config, 20, root_entity );
}

PlayerGameplaySystem::~PlayerGameplaySystem() 
{
}

void
PlayerGameplaySystem::Update( float delta_time_sec )
{
	// Get only entities with PlayerComponent (much smaller set)
	for( auto& [entity, player] : mRegistry.GetComponentMap<PlayerComponent>() )
	{
		if( !mRegistry.HasComponent<HealthComponent>( entity ) )
		{
			LOG_ERROR() << "Player entity " << entity << " is missing HealthComponent.";
			continue; // Shouldn't happen, but just in case
		}

		// Process incoming damage
		auto& health = mRegistry.GetComponent<HealthComponent>( entity );
		bool is_dead = health.IsDead();
		if( health.pending_damage > 0.f )
		{
			health.health -= health.pending_damage;
			health.pending_damage = 0.f;
			LOG() << "Player health: " << health.health;

			if( is_dead )
			{
				player.thruster_on = false;
				LOG() << "Player has died.";

				// Zero out physics velocity
				if( mRegistry.HasComponent<eage::ecs::PhysicsComponent>( entity ) )
				{
					auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
					physics.QueueSetVelocity( glm::vec2( 0.f, 0.f ) );
					physics.QueueSetAngularVelocity( 0.f );
				}
			}
		}

		UpdateThrusterFX( player, entity );

		// Skip movement and weapon when dead
		if( is_dead )
		{
			continue;
		}

		// We know this entity has PlayerComponent, now check for others
		if( mRegistry.HasComponent<eage::ecs::PhysicsComponent>( entity ) &&
			mRegistry.HasComponent<eage::ecs::TransformComponent>( entity ) )
		{
			auto& physics = mRegistry.GetComponent<eage::ecs::PhysicsComponent>( entity );
			auto& transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( entity );
			
			UpdatePlayerMovement( player, physics, transform, delta_time_sec );
			UpdateWeapon( player, physics, transform );
		}
	}
}

void
PlayerGameplaySystem::UpdateThrusterFX( PlayerComponent& player_comp, uint64_t entity )
{
	if( mRegistry.HasComponent<HealthComponent>( entity ) )
	{
		player_comp.thruster_on = player_comp.thruster_on && !mRegistry.GetComponent<HealthComponent>( entity ).IsDead();
	}
	if( player_comp.thruster_fx_entity != 0 &&
		mRegistry.HasComponent<eage::ecs::RenderComponent>( player_comp.thruster_fx_entity ) )
	{
		mRegistry.GetComponent<eage::ecs::RenderComponent>( player_comp.thruster_fx_entity ).visible = player_comp.thruster_on;
	}

	if( mRegistry.HasComponent<AudioEventComponent>( entity ) )
	{
		auto& audio_event = mRegistry.GetComponent<AudioEventComponent>( entity );
		audio_event.QueueEvent( "thruster", player_comp.thruster_on
			? AudioEventComponent::EventType::Play
			: AudioEventComponent::EventType::Stop );
	}
}

void
PlayerGameplaySystem::UpdatePlayerMovement( PlayerComponent& player_comp, 
											eage::ecs::PhysicsComponent& physics_comp,
											eage::ecs::TransformComponent& transform_comp,
											float delta_time_sec )
{
	// Rotation
	if( player_comp.turning_left )
	{
		physics_comp.QueueSetAngularVelocity( player_comp.rotation_speed ); // Convert to radians
	} 
	else if( player_comp.turning_right )
	{
		physics_comp.QueueSetAngularVelocity( -player_comp.rotation_speed ); // Convert to radians
	}
	else
	{
		physics_comp.QueueSetAngularVelocity( 0.0f );
	}

	glm::vec3 forward = transform_comp.rotation * player_comp.forward;

	// Thrust
	if( player_comp.thruster_on )
	{
		glm::vec2 velocity_change = glm::vec2( forward.x, forward.y ) * player_comp.thrust_acceleration * delta_time_sec;
		physics_comp.QueueAddVelocity( velocity_change );
	}
}

void
PlayerGameplaySystem::UpdateWeapon( PlayerComponent& player_comp,
									eage::ecs::PhysicsComponent& physics_comp,
									eage::ecs::TransformComponent& transform_comp )
{
	if( !player_comp.main_weapon_firing )
	{
		return;
	}

	// Consume the fire input (single-shot per key press)
	player_comp.main_weapon_firing = false;

	if( player_comp.bullet_launcher_entity == 0 ||
		!mRegistry.HasComponent<eage::ecs::TransformComponent>( player_comp.bullet_launcher_entity ) )
	{
		return;
	}

	auto& launcher_transform = mRegistry.GetComponent<eage::ecs::TransformComponent>( player_comp.bullet_launcher_entity );
	glm::vec2 spawn_pos = glm::vec2( launcher_transform.world_matrix[3] );

	glm::vec3 forward = transform_comp.rotation * player_comp.forward;
	glm::vec2 fire_dir = glm::normalize( glm::vec2( forward.x, forward.y ) );

	float bullet_speed = physics_comp.max_linear_velocity + 20.f;

	bool fired = mBulletSystem.Fire( mDefaultBulletPoolId, spawn_pos, fire_dir, bullet_speed );

	if( fired && mRegistry.HasComponent<AudioEventComponent>( player_comp.bullet_launcher_entity ) )
	{
		auto& audio_event = mRegistry.GetComponent<AudioEventComponent>( player_comp.bullet_launcher_entity );
		audio_event.QueueEvent( "fire", AudioEventComponent::EventType::Play );
	}
}
