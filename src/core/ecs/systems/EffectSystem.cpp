#include "EffectSystem.h"

#include <assets/AnimationClip.h>
#include <ecs/ECS.h>
#include <ecs/components/Audio.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Effect.h>
#include <ecs/components/Render.h>
#include <ecs/systems/AnimationSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/systems/SceneGraphSystem.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace utility;

namespace
{
	constexpr float INACTIVE_OFFSET = 2000.0f;
	constexpr const char* EFFECT_SFX_SOURCE = "sfx";
}

EffectSystem::EffectSystem( ECSRegistry& registry, AnimationSystem& animation_system,
							SceneGraphSystem& scene_graph_system )
	: mRegistry( registry )
	, mAnimationSystem( animation_system )
	, mSceneGraphSystem( scene_graph_system )
{
}

EffectSystem::~EffectSystem() = default;

ResourceId
EffectSystem::Create( RenderSystem& render_system, const EffectConfig& config, Entity root_entity )
{
	if( config.clip_ids.empty() || config.clip_ids[0] == INVALID_ID )
	{
		LOG_ERROR() << "EffectSystem::Create: clip_ids must be non-empty with a valid primary clip";
		return INVALID_ID;
	}

	if( config.material_id == INVALID_ID )
	{
		LOG_ERROR() << "EffectSystem::Create: material_id is required";
		return INVALID_ID;
	}

	if( config.pool_size <= 0 )
	{
		LOG_ERROR() << "EffectSystem::Create: pool_size must be > 0";
		return INVALID_ID;
	}

	const assets::AnimationClip* clip = mAnimationSystem.GetClip( config.clip_ids[0] );
	if( clip == nullptr || clip->GetFrameCount() == 0 )
	{
		LOG_ERROR() << "EffectSystem::Create: primary clip is invalid or empty";
		return INVALID_ID;
	}

	float mesh_width = 32.f;
	float mesh_height = 32.f;
	const glm::ivec2 frame_size = clip->GetFrameSize();
	if( frame_size.x > 0 && frame_size.y > 0 )
	{
		mesh_width = static_cast<float>( frame_size.x );
		mesh_height = static_cast<float>( frame_size.y );
	}

	const uint32_t texture_index = clip->GetFrameTexture( 0 );
	const ResourceId mesh_id = render_system.CreateSpriteMesh( mesh_width, mesh_height );

	ResourceId effect_id = mNextEffectId++;
	EffectDefinition& definition = mEffects[effect_id];
	definition.clip_ids = config.clip_ids;
	definition.material_id = config.material_id;
	definition.mesh_id = mesh_id;
	definition.sound_id = config.sound_id;

	const glm::vec2 inactive_pos( INACTIVE_OFFSET, -INACTIVE_OFFSET );

	for( int i = 0; i < config.pool_size; ++i )
	{
		Entity entity = mRegistry.CreateEntity();
		definition.available.push_back( entity );
		definition.all.insert( entity );

		mSceneGraphSystem.AddNodeToParent( entity, root_entity );

		TransformComponent transform;
		transform.SetPosition( glm::vec3( inactive_pos, 0.f ) );
		mRegistry.AddComponent( entity, std::move( transform ) );

		mRegistry.AddComponent( entity, EffectInstanceComponent{ effect_id, false } );

		render_system.AttachRenderable( entity, mesh_id, config.material_id, texture_index, false );

		mAnimationSystem.Attach( entity, config.clip_ids[0] );
		mAnimationSystem.Pause( entity );

		if( config.sound_id != INVALID_ID )
		{
			AudioSourceComponent audio_source;
			audio_source.sources[EFFECT_SFX_SOURCE] = { config.sound_id };
			mRegistry.AddComponent( entity, std::move( audio_source ) );
			mRegistry.AddComponent( entity, AudioEventComponent{} );
		}
	}

	return effect_id;
}

bool
EffectSystem::Apply( ResourceId effect_id, glm::vec2 pos, const glm::quat& rotation )
{
	auto it = mEffects.find( effect_id );
	if( it == mEffects.end() )
	{
		LOG_ERROR() << "EffectSystem::Apply: unknown effect id " << effect_id;
		return false;
	}

	EffectDefinition& definition = it->second;
	if( definition.available.empty() )
	{
		LOG() << "EffectSystem::Apply: pool exhausted for effect id " << effect_id;
		return false;
	}

	if( definition.clip_ids.empty() || definition.clip_ids[0] == INVALID_ID )
	{
		LOG_ERROR() << "EffectSystem::Apply: effect has no primary clip";
		return false;
	}

	Entity entity = definition.available.front();
	definition.available.pop_front();

	auto& instance = mRegistry.GetComponent<EffectInstanceComponent>( entity );
	instance.active = true;

	auto& transform = mRegistry.GetComponent<TransformComponent>( entity );
	transform.SetPosition( glm::vec3( pos, 0.f ) );
	transform.SetRotation( rotation );

	auto& render = mRegistry.GetComponent<RenderComponent>( entity );
	render.visible = true;

	mAnimationSystem.Attach( entity, definition.clip_ids[0] );
	mAnimationSystem.PlayOnce( entity, 0 );

	if( mRegistry.HasComponent<AudioEventComponent>( entity ) )
	{
		mRegistry.GetComponent<AudioEventComponent>( entity )
			.QueueEvent( EFFECT_SFX_SOURCE, AudioEventComponent::EventType::Play );
	}

	return true;
}

void
EffectSystem::Update()
{
	for( auto [entity, instance] : mRegistry.GetComponentMap<EffectInstanceComponent>() )
	{
		if( !instance.active )
		{
			continue;
		}

		if( !mAnimationSystem.IsFinished( entity ) )
		{
			continue;
		}

		auto it = mEffects.find( instance.effect_id );
		if( it == mEffects.end() )
		{
			continue;
		}

		Recycle( entity, it->second );
	}
}

void
EffectSystem::Recycle( Entity entity, EffectDefinition& definition )
{
	auto& instance = mRegistry.GetComponent<EffectInstanceComponent>( entity );
	instance.active = false;

	auto& render = mRegistry.GetComponent<RenderComponent>( entity );
	render.visible = false;

	auto& transform = mRegistry.GetComponent<TransformComponent>( entity );
	transform.SetPosition( glm::vec3( INACTIVE_OFFSET, -INACTIVE_OFFSET, 0.f ) );

	if( mAnimationSystem.HasAnimation( entity ) )
	{
		mAnimationSystem.ShowFrame( entity, 0 );
		mAnimationSystem.Pause( entity );
	}

	definition.available.push_back( entity );
}
