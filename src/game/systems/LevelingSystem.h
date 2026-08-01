#ifndef VORTEX_LEVELING_SYSTEM_H
#define VORTEX_LEVELING_SYSTEM_H

namespace eage::ecs
{
	class ECSRegistry;
}

namespace vortex
{
	///
	/// LevelingSystem: Applies pending XP and handles level-up thresholds.
	///
	class LevelingSystem
	{
	public:
		explicit LevelingSystem( eage::ecs::ECSRegistry& registry );

		void Update();

	private:
		eage::ecs::ECSRegistry& mRegistry;
	};
} // namespace vortex

#endif // VORTEX_LEVELING_SYSTEM_H
