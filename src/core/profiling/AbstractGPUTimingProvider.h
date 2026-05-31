#ifndef _EAGE_PROFILING_ABSTRACT_GPU_TIMING_PROVIDER_H_
#define _EAGE_PROFILING_ABSTRACT_GPU_TIMING_PROVIDER_H_

namespace eage::profiling
{
	///
	/// AbstractGPUTimingProvider: Abstracts GPU frame-time queries away from a concrete backend.
	/// Implement this on any renderer that can report GPU timing.
	///
	class AbstractGPUTimingProvider
	{
	public:
		virtual ~AbstractGPUTimingProvider() = default;

		virtual float GetGPUFrameTime() const = 0;
	};
}

#endif // _EAGE_PROFILING_ABSTRACT_GPU_TIMING_PROVIDER_H_
