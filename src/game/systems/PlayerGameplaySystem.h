#ifndef _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H
#define _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H

#include <ecs/ECS.h>

#include <glm/glm.hpp>

namespace eage::ecs
{
	struct TransformComponent;
	struct Velocity2DComponent;
}

namespace vortex
{
	struct PlayerComponent;

	class PlayerGameplaySystem 
	{
	public:
		PlayerGameplaySystem( eage::ecs::ECSRegistry& registry );
		~PlayerGameplaySystem();
		
		void Update( float delta_time );
		
	private:
		void UpdatePlayerMovement( PlayerComponent& player_comp,
								   eage::ecs::Velocity2DComponent& velocity_comp,
								   eage::ecs::TransformComponent& transform_comp,
								   float delta_time_sec );
		
		eage::ecs::ECSRegistry& mRegistry;
	};
}

#endif // _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H