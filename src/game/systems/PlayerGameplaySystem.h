#ifndef _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H
#define _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H

#include <ecs/ECS.h>

#include <glm/glm.hpp>

#include "BulletSystem.h"

namespace eage::ecs
{
	struct PhysicsComponent;
	struct TransformComponent;
}

namespace vortex
{
	struct PlayerComponent;

	///
	/// PlayerGameplaySystem: Updates player movement and gameplay logic
	///
	class PlayerGameplaySystem 
	{
	public:
		PlayerGameplaySystem( eage::ecs::ECSRegistry& registry, BulletSystem& bullet_system, BulletPoolId player_bullet_pool_id );
		~PlayerGameplaySystem();
		
		void Update( float delta_time );
		
	private:
		void UpdatePlayerMovement( PlayerComponent& player_comp,
								   eage::ecs::PhysicsComponent& physics_comp,
								   eage::ecs::TransformComponent& transform_comp,
								   float delta_time_sec );
		void UpdateThrusterFX( PlayerComponent& player_comp, uint64_t entity );
		void UpdateWeapon( PlayerComponent& player_comp,
						   eage::ecs::PhysicsComponent& physics_comp,
						   eage::ecs::TransformComponent& transform_comp );

		eage::ecs::ECSRegistry& mRegistry;
		BulletSystem& mBulletSystem;
		BulletPoolId mPlayerBulletPoolId;
	};
}

#endif // _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H