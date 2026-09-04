#ifndef _WAVE_SYSTEM_H
#define _WAVE_SYSTEM_H

#include <vector>

#include <utility/Pausable.h>

#include "../data/WaveTypes.h"

namespace vortex
{
	class EnemySystem;
	class WaveStore;

	///
	/// WaveSystem: owns wave sequencing, batch spawn timers, and completion.
	/// Next wave starts on the following frame after timeout or wipe.
	///
	class WaveSystem : public utility::Pausable
	{
	public:
		explicit WaveSystem( EnemySystem& enemy_system );

		void SetStore( const WaveStore& store );
		void StartWave( int wave_index );
		void Update( float dt );

		WaveHudState GetHudState() const;

	private:
		enum class State
		{
			IDLE,
			RUNNING,
			PENDING_NEXT,
			FINISHED
		};

		struct GroupRuntime
		{
			int spawned = 0;
			float time_since_batch = 0.f;
		};

		void TickBatches( float dt, bool is_wave_start );
		void SpawnGroupBatch( int group_index, bool spawn_remaining );
		bool IsWaveCleared() const;
		void EndCurrentWave( bool despawn_survivors );

		EnemySystem& mEnemySystem;
		std::vector<WaveDefinition> mWaves;
		std::vector<GroupRuntime> mGroups;
		State mState = State::IDLE;
		int mCurrentWaveIndex = -1;
		int mPendingWaveIndex = -1;
		float mTimeRemaining = 0.f;
		float mBannerTimeRemaining = 0.f;
	};
}

#endif // _WAVE_SYSTEM_H
