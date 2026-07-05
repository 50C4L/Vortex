#include "AnimatedSprite.h"

#include <algorithm>

#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <utility/Logger.h>

using namespace eage::animation;
using namespace eage::ecs;
using namespace assets;
using namespace utility;


AnimatedSprite::AnimatedSprite(
	const AnimationClip& clip,
	Entity entity,
	ECSRegistry& registry,
	RenderSystem* render_system,
	ResourceId material_id )
	: mClip( clip )
	, mEntity( entity )
	, mRegistry( registry )
	, mRenderSystem( render_system )
	, mMaterialId( material_id )
{
	if( mClip.UsesPerFrameTextures() )
	{
		if( !mRenderSystem || mMaterialId == INVALID_ID )
		{
			LOG_ERROR( "AnimatedSprite: per-frame texture clips require RenderSystem and material id." );
		}
		else
		{
			for( int frame_index = 0; frame_index < mClip.GetFrameCount(); ++frame_index )
			{
				const std::string texture_path = mClip.GetResolvedTexturePath( frame_index );
				if( !texture_path.empty() )
				{
					mRenderSystem->CreateImageBuffer( texture_path );
				}
			}
		}
	}

	if( mClip.GetFrameCount() > 0 )
	{
		ShowFrame( 0 );
	}
}

AnimatedSprite::~AnimatedSprite() = default;

void
AnimatedSprite::Play()
{
	mPlaying = true;
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
	render_cmp.uv_rect =
	{
		frame.uv_min.x,
		frame.uv_min.y,
		frame.uv_max.x - frame.uv_min.x,
		frame.uv_max.y - frame.uv_min.y
	};

	if( mClip.UsesPerFrameTextures() && mRenderSystem && mMaterialId != INVALID_ID )
	{
		const std::string texture_path = mClip.GetResolvedTexturePath( mCurrentFrame );
		if( !texture_path.empty() )
		{
			mRenderSystem->SetMaterialTexture( mMaterialId, texture_path );
		}
	}
}

void
AnimatedSprite::SetLoop( bool loop )
{
	mLoop = loop;
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
			mPlaying = false;
			return;
		}
	}

	ShowFrame( next_frame );
}
