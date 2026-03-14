#ifndef VORTEX_WARP_SYSTEM_H
#define VORTEX_WARP_SYSTEM_H

#include <ecs/systems/PhysicsSystem.h>

namespace eage::ecs
{
	class ECSRegistry;
}

namespace vortex
{
	struct WarpBoundaryComponent;

	///
	/// WarpSystem: Handles warping entities with WarpComponent when they exit screen bounds
	///
	class WarpSystem final : public eage::ecs::PhysicsSystem::Observer
	{
	public:
		WarpSystem( eage::ecs::ECSRegistry& registry, eage::ecs::PhysicsSystem& physics_system );
		
		~WarpSystem();

		void SetScreenEntity( uint64_t screen_entity );
		
		// PhysicsSystem::Observer interface
		void OnSensorEnter( uint64_t sensor, uint64_t visitor ) override;
		void OnSensorExit( uint64_t sensor, uint64_t visitor ) override;
		void OnCollideBegin( uint64_t entityA, uint64_t entityB ) override;
		void OnCollideEnd( uint64_t entityA, uint64_t entityB ) override;
		
	private:
		glm::vec2 CalculateWrapPosition( const glm::vec2& exit_pos, const WarpBoundaryComponent& boundary );

		eage::ecs::ECSRegistry& mRegistry;
		eage::ecs::PhysicsSystem& mPhysicsSystem;

		uint64_t mScreenEntity = 0; // Entity representing the screen bounds
	};
} // namespace vortex

#endif // VORTEX_WARP_SYSTEM_H