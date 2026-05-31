#ifndef _EAGE_PROFILING_PERFORMANCE_TRACKER_H_
#define _EAGE_PROFILING_PERFORMANCE_TRACKER_H_

#include <chrono>

#include "AbstractGPUTimingProvider.h"

namespace eage::profiling
{
	class PerformanceTracker
	{
	public:
		PerformanceTracker( AbstractGPUTimingProvider& gpu_timing_provider );
		
		void Update();
		void DrawDebugGUI();
		
		void SetEnabled(bool enabled) { mEnabled = enabled; }
		bool IsEnabled() const { return mEnabled; }
		
		float GetFPS() const { return mFPS; }
		float GetCPUFrameTime() const { return mCPUFrameTime; }
		float GetGPUFrameTime() const { return mGPUFrameTime; }
		
	private:
		AbstractGPUTimingProvider& mGPUTimingProvider;

		bool mEnabled = true;
		float mFPS = 0.0f;
		float mCPUFrameTime = 0.0f;
		float mGPUFrameTime = 0.0f;
		
		std::chrono::steady_clock::time_point mLastFrameTime;
		
		// Moving average for smoother readings
		static constexpr size_t FRAME_HISTORY_SIZE = 60;
		float mFrameTimeHistory[FRAME_HISTORY_SIZE] = {};
		size_t mFrameHistoryIndex = 0;
	};
}

#endif // _EAGE_PROFILING_PERFORMANCE_TRACKER_H_