#include "AnimationSystem.h"

#include <algorithm>

#include <assets/AnimationClip.h>
#include <ecs/ECS.h>
#include <ecs/components/Animation.h>
#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace assets;
using namespace utility;

AnimationSystem::AnimationSystem( ECSRegistry& registry )
	: mRegistry( registry )
{
}

AnimationSystem::~AnimationSystem() = default;

void
AnimationSystem::Update( float delta_time )
{
	for( auto [entity, anim] : mRegistry.GetComponentMap<AnimatedSpriteComponent>() )
	{
		if( !anim.playing || anim.clip_id == INVALID_ID )
		{
			continue;
		}

		const AnimationClip* clip = GetClip( anim.clip_id );
		if( clip == nullptr || clip->GetFrameCount() == 0 )
		{
			continue;
		}

		anim.elapsed += delta_time;

		const float frame_duration = clip->GetFrame( anim.current_frame ).duration_sec;
		if( anim.elapsed < frame_duration )
		{
			continue;
		}

		anim.elapsed -= frame_duration;

		int next_frame = anim.current_frame + 1;
		if( next_frame >= clip->GetFrameCount() )
		{
			if( anim.loop )
			{
				next_frame = 0;
			}
			else
			{
				anim.current_frame = clip->GetFrameCount() - 1;
				ShowFrame( anim, entity );
				anim.playing = false;
				anim.finished = true;
				continue;
			}
		}

		anim.current_frame = std::clamp( next_frame, 0, clip->GetFrameCount() - 1 );
		ShowFrame( anim, entity );
	}
}

ResourceId
AnimationSystem::LoadClip( RenderSystem& render_system, const std::string& clip_json_path )
{
	auto path_it = mClipPathToId.find( clip_json_path );
	if( path_it != mClipPathToId.end() )
	{
		return path_it->second;
	}

	auto loaded_clip = AnimationClip::Load( render_system, clip_json_path );
	if( loaded_clip == nullptr || loaded_clip->GetFrameCount() == 0 )
	{
		LOG_ERROR() << "AnimationSystem: failed to load clip from " << clip_json_path;
		return INVALID_ID;
	}

	std::shared_ptr<const AnimationClip> clip = std::move( loaded_clip );
	ResourceId clip_id = mClipStore.Store( std::move( clip ) );
	mClipPathToId[clip_json_path] = clip_id;
	return clip_id;
}

const AnimationClip*
AnimationSystem::GetClip( ResourceId clip_id ) const
{
	return mClipStore.Get( clip_id );
}

void
AnimationSystem::Attach( Entity entity, ResourceId clip_id )
{
	if( clip_id == INVALID_ID )
	{
		return;
	}

	if( !mRegistry.HasComponent<AnimatedSpriteComponent>( entity ) )
	{
		mRegistry.AddComponent( entity, AnimatedSpriteComponent{ clip_id } );
	}
	else
	{
		auto& anim = mRegistry.GetComponent<AnimatedSpriteComponent>( entity );
		anim.clip_id = clip_id;
		anim.current_frame = 0;
		anim.elapsed = 0.f;
		anim.playing = false;
		anim.loop = true;
		anim.finished = false;
	}

	ShowFrame( entity, 0 );
	Pause( entity );
}

void
AnimationSystem::Play( Entity entity, int start_frame )
{
	auto* anim = GetAnimComponent( entity );
	if( anim == nullptr || anim->clip_id == INVALID_ID )
	{
		return;
	}

	const AnimationClip* clip = GetClip( anim->clip_id );
	if( clip == nullptr || clip->GetFrameCount() == 0 )
	{
		return;
	}

	anim->finished = false;
	anim->elapsed = 0.f;
	anim->playing = true;
	ShowFrame( entity, start_frame );
}

void
AnimationSystem::PlayOnce( Entity entity, int start_frame )
{
	SetLoop( entity, false );
	Play( entity, start_frame );
}

void
AnimationSystem::Pause( Entity entity )
{
	auto* anim = GetAnimComponent( entity );
	if( anim == nullptr )
	{
		return;
	}

	anim->playing = false;
}

void
AnimationSystem::ShowFrame( Entity entity, int index )
{
	auto* anim = GetAnimComponent( entity );
	if( anim == nullptr || anim->clip_id == INVALID_ID )
	{
		return;
	}

	const AnimationClip* clip = GetClip( anim->clip_id );
	if( clip == nullptr || clip->GetFrameCount() == 0 )
	{
		return;
	}

	anim->current_frame = std::clamp( index, 0, clip->GetFrameCount() - 1 );
	ShowFrame( *anim, entity );
}

void
AnimationSystem::SetLoop( Entity entity, bool loop )
{
	auto* anim = GetAnimComponent( entity );
	if( anim == nullptr )
	{
		return;
	}

	anim->loop = loop;
}

bool
AnimationSystem::IsPlaying( Entity entity ) const
{
	const auto* anim = GetAnimComponent( entity );
	return anim != nullptr && anim->playing;
}

bool
AnimationSystem::IsFinished( Entity entity ) const
{
	const auto* anim = GetAnimComponent( entity );
	return anim != nullptr && anim->finished;
}

bool
AnimationSystem::HasAnimation( Entity entity ) const
{
	return GetAnimComponent( entity ) != nullptr;
}

void
AnimationSystem::ShowFrame( AnimatedSpriteComponent& anim, Entity entity )
{
	if( anim.clip_id == INVALID_ID )
	{
		return;
	}

	const AnimationClip* clip = GetClip( anim.clip_id );
	if( clip == nullptr || clip->GetFrameCount() == 0 )
	{
		return;
	}

	anim.current_frame = std::clamp( anim.current_frame, 0, clip->GetFrameCount() - 1 );

	if( !mRegistry.HasComponent<RenderComponent>( entity ) )
	{
		return;
	}

	const auto& frame = clip->GetFrame( anim.current_frame );
	auto& render_cmp = mRegistry.GetComponent<RenderComponent>( entity );
	render_cmp.texture_index = frame.texture_index;
	render_cmp.uv_rect = { 0.f, 0.f, 1.f, 1.f };
}

AnimatedSpriteComponent*
AnimationSystem::GetAnimComponent( Entity entity )
{
	if( !mRegistry.HasComponent<AnimatedSpriteComponent>( entity ) )
	{
		return nullptr;
	}

	return &mRegistry.GetComponent<AnimatedSpriteComponent>( entity );
}

const AnimatedSpriteComponent*
AnimationSystem::GetAnimComponent( Entity entity ) const
{
	if( !mRegistry.HasComponent<AnimatedSpriteComponent>( entity ) )
	{
		return nullptr;
	}

	return &mRegistry.GetComponent<AnimatedSpriteComponent>( entity );
}
