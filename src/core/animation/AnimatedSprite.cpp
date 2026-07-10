#include "AnimatedSprite.h"

#include <algorithm>

#include <ecs/components/Render.h>

using namespace eage::animation;
using namespace eage::ecs;
using namespace assets;


AnimatedSprite::AnimatedSprite( const AnimationClip& clip, Entity entity, ECSRegistry& registry )
	: mClip( clip )
	, mEntity( entity )
	, mRegistry( registry )
{
	if( mClip.GetFrameCount() > 0 )
	{
		ShowFrame( 0 );
	}
}

AnimatedSprite::~AnimatedSprite() = default;

void
AnimatedSprite::Play( int start_frame )
{
	if( mClip.GetFrameCount() == 0 )
	{
		return;
	}

	mFinished = false;
	mElapsed = 0.f;
	mPlaying = true;
	ShowFrame( start_frame );
}

void
AnimatedSprite::PlayOnce( int start_frame )
{
	SetLoop( false );
	Play( start_frame );
}

void
AnimatedSprite::Pause()
{
	mPlaying = false;
}

void
AnimatedSprite::ShowFrame( int index )
{
	if( mClip.GetFrameCount() == 0 )
	{
		return;
	}

	mCurrentFrame = std::clamp( index, 0, mClip.GetFrameCount() - 1 );

	if( !mRegistry.HasComponent<RenderComponent>( mEntity ) )
	{
		return;
	}

	const auto& frame = mClip.GetFrame( mCurrentFrame );
	auto& render_cmp = mRegistry.GetComponent<RenderComponent>( mEntity );
	render_cmp.texture_index = frame.texture_index;
	render_cmp.uv_rect = { 0.f, 0.f, 1.f, 1.f };
}

void
AnimatedSprite::SetLoop( bool loop )
{
	mLoop = loop;
}

bool
AnimatedSprite::IsPlaying() const
{
	return mPlaying;
}

bool
AnimatedSprite::IsFinished() const
{
	return mFinished;
}

void
AnimatedSprite::Update( float delta_time_sec )
{
	if( !mPlaying || mClip.GetFrameCount() == 0 )
	{
		return;
	}

	mElapsed += delta_time_sec;

	const float frame_duration = mClip.GetFrame( mCurrentFrame ).duration_sec;
	if( mElapsed < frame_duration )
	{
		return;
	}

	mElapsed -= frame_duration;

	int next_frame = mCurrentFrame + 1;
	if( next_frame >= mClip.GetFrameCount() )
	{
		if( mLoop )
		{
			next_frame = 0;
		}
		else
		{
			ShowFrame( mClip.GetFrameCount() - 1 );
			mPlaying = false;
			mFinished = true;
			return;
		}
	}

	ShowFrame( next_frame );
}
