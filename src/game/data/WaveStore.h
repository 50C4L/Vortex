#ifndef _WAVE_STORE_H
#define _WAVE_STORE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "WaveTypes.h"

namespace vortex
{
	///
	/// WaveStore: parses and validates waves.json into typed wave definitions.
	/// Pure data -- no ECS, rendering, or physics.
	///
	class WaveStore
	{
	public:
		bool Load( const std::string& path );

		const std::vector<WaveDefinition>& GetWaves() const;

		/// Worst-case concurrent count per enemy id across all waves.
		std::unordered_map<std::string, int> ComputePoolRequirements() const;

	private:
		std::vector<WaveDefinition> mWaves;
	};
}

#endif // _WAVE_STORE_H
