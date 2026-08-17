#ifndef _DELTA_TIMER_H
#define _DELTA_TIMER_H

#include <chrono>

namespace utility
{
	///
	/// Wall-clock seconds since the previous Tick() (or since construction / Reset()).
	/// Each consumer should own its own DeltaTimer so later systems include time spent
	/// by systems that already ran this frame.
	///
	class DeltaTimer
	{
	public:
		DeltaTimer()
			: mLast( std::chrono::steady_clock::now() )
		{
		}

		float Tick()
		{
			const auto now = std::chrono::steady_clock::now();
			const float dt = std::chrono::duration<float>( now - mLast ).count();
			mLast = now;
			return dt;
		}

		void Reset()
		{
			mLast = std::chrono::steady_clock::now();
		}

	private:
		std::chrono::steady_clock::time_point mLast;
	};
}

#endif // _DELTA_TIMER_H
