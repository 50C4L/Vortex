#include "WaveSystem.h"

#include <cmath>
#include <string>

#include <utility/Logger.h>

#include "../data/WaveStore.h"
#include "EnemySystem.h"

using namespace vortex;
using namespace utility;

namespace
{
	constexpr float BANNER_DURATION_SEC = 2.f;
}

WaveSystem::WaveSystem( EnemySystem& enemy_system )
	: mEnemySystem( enemy_system )
{
}

void
WaveSystem::SetStore( const WaveStore& store )
{
	mWaves = store.GetWaves();
	mGroups.clear();
	mState = State::IDLE;
	mCurrentWaveIndex = -1;
	mPendingWaveIndex = -1;
	mTimeRemaining = 0.f;
	mBannerTimeRemaining = 0.f;
}

void
WaveSystem::StartWave( int wave_index )
{
	if( wave_index < 0 || wave_index >= static_cast<int>( mWaves.size() ) )
	{
		LOG_ERROR() << "WaveSystem::StartWave: invalid wave index " << wave_index;
		mState = State::FINISHED;
		mTimeRemaining = 0.f;
		mBannerTimeRemaining = 0.f;
		mGroups.clear();
		return;
	}

	mCurrentWaveIndex = wave_index;
	mPendingWaveIndex = -1;
	mState = State::RUNNING;

	const WaveDefinition& wave = mWaves[static_cast<size_t>( wave_index )];
	mTimeRemaining = wave.time_sec;
	mBannerTimeRemaining = BANNER_DURATION_SEC;

	mGroups.clear();
	mGroups.resize( wave.enemies.size() );

	TickBatches( 0.f, true );
}

void
WaveSystem::Update( float dt )
{
	if( mState == State::PENDING_NEXT )
	{
		StartWave( mPendingWaveIndex );
	}

	if( mState != State::RUNNING )
	{
		return;
	}

	mTimeRemaining -= dt;
	if( mBannerTimeRemaining > 0.f )
	{
		mBannerTimeRemaining -= dt;
		if( mBannerTimeRemaining < 0.f )
		{
			mBannerTimeRemaining = 0.f;
		}
	}

	TickBatches( dt, false );

	if( mTimeRemaining <= 0.f )
	{
		EndCurrentWave( true );
		return;
	}

	if( IsWaveCleared() )
	{
		EndCurrentWave( false );
	}
}

WaveHudState
WaveSystem::GetHudState() const
{
	WaveHudState hud;
	if( mCurrentWaveIndex < 0 )
	{
		return hud;
	}

	hud.wave_number = mCurrentWaveIndex + 1;
	if( mState == State::RUNNING )
	{
		hud.time_remaining_sec = static_cast<int>( std::ceil( mTimeRemaining ) );
		if( hud.time_remaining_sec < 0 )
		{
			hud.time_remaining_sec = 0;
		}
		hud.banner_visible = mBannerTimeRemaining > 0.f;
		if( hud.banner_visible )
		{
			hud.banner_text = "Wave " + std::to_string( hud.wave_number );
		}
	}

	return hud;
}

void
WaveSystem::TickBatches( float dt, bool is_wave_start )
{
	if( mCurrentWaveIndex < 0 || mCurrentWaveIndex >= static_cast<int>( mWaves.size() ) )
	{
		return;
	}

	const WaveDefinition& wave = mWaves[static_cast<size_t>( mCurrentWaveIndex )];
	for( size_t i = 0; i < wave.enemies.size(); ++i )
	{
		const SpawnGroup& group = wave.enemies[i];
		GroupRuntime& runtime = mGroups[i];
		const int remaining = group.total - runtime.spawned;
		if( remaining <= 0 )
		{
			continue;
		}

		if( group.spawn_interval <= 0.f )
		{
			if( is_wave_start )
			{
				SpawnGroupBatch( static_cast<int>( i ), true );
			}
			continue;
		}

		if( is_wave_start )
		{
			SpawnGroupBatch( static_cast<int>( i ), false );
			runtime.time_since_batch = 0.f;
			continue;
		}

		runtime.time_since_batch += dt;
		while( runtime.time_since_batch >= group.spawn_interval && runtime.spawned < group.total )
		{
			runtime.time_since_batch -= group.spawn_interval;
			SpawnGroupBatch( static_cast<int>( i ), false );
		}
	}
}

void
WaveSystem::SpawnGroupBatch( int group_index, bool spawn_remaining )
{
	const WaveDefinition& wave = mWaves[static_cast<size_t>( mCurrentWaveIndex )];
	const SpawnGroup& group = wave.enemies[static_cast<size_t>( group_index )];
	GroupRuntime& runtime = mGroups[static_cast<size_t>( group_index )];

	const int remaining = group.total - runtime.spawned;
	if( remaining <= 0 )
	{
		return;
	}

	const int count = spawn_remaining ? remaining : ( remaining < group.batch_size ? remaining : group.batch_size );
	if( !mEnemySystem.Spawn( group.id, count ) )
	{
		LOG_ERROR() << "WaveSystem: failed to spawn " << count << " of " << group.id;
		return;
	}

	runtime.spawned += count;
}

bool
WaveSystem::IsWaveCleared() const
{
	if( mCurrentWaveIndex < 0 || mCurrentWaveIndex >= static_cast<int>( mWaves.size() ) )
	{
		return false;
	}

	const WaveDefinition& wave = mWaves[static_cast<size_t>( mCurrentWaveIndex )];
	if( mGroups.size() != wave.enemies.size() )
	{
		return false;
	}

	for( size_t i = 0; i < wave.enemies.size(); ++i )
	{
		if( mGroups[i].spawned < wave.enemies[i].total )
		{
			return false;
		}
	}

	return mEnemySystem.GetLiveCount() == 0;
}

void
WaveSystem::EndCurrentWave( bool despawn_survivors )
{
	if( despawn_survivors )
	{
		mEnemySystem.DespawnAll();
	}

	mGroups.clear();
	mTimeRemaining = 0.f;
	mBannerTimeRemaining = 0.f;

	const int next_index = mCurrentWaveIndex + 1;
	if( next_index < static_cast<int>( mWaves.size() ) )
	{
		mPendingWaveIndex = next_index;
		mState = State::PENDING_NEXT;
		return;
	}

	mState = State::FINISHED;
}
