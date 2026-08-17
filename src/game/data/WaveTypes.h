#ifndef _WAVE_TYPES_H
#define _WAVE_TYPES_H

#include <string>
#include <vector>

namespace vortex
{
	enum class SpawnType
	{
		BATCH
	};

	struct SpawnGroup
	{
		std::string id;
		int total = 0;
		SpawnType spawn_type = SpawnType::BATCH;
		int batch_size = 0;
		float spawn_interval = 0.f;
	};

	struct WaveDefinition
	{
		float time_sec = 0.f;
		std::vector<SpawnGroup> enemies;
	};

	struct WaveHudState
	{
		int wave_number = 0;
		int time_remaining_sec = 0;
		bool banner_visible = false;
		std::string banner_text;
	};
}

#endif // _WAVE_TYPES_H
