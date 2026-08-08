#ifndef _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H
#define _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H

#include <ecs/ECS.h>

#include <glm/glm.hpp>
#include <memory>

namespace assets
{
	class SceneResourceLoader;
}

namespace eage::ecs
{
	class AudioSystem;
	class RenderSystem;
	class SceneGraphSystem;
	struct PhysicsComponent;
	struct TransformComponent;
}

namespace vortex
{
	class BulletSystem;
	struct PlayerComponent;

	///
	/// PlayerGameplaySystem: Updates player movement and gameplay logic.
	/// Call PreparePlayer() after construction to create all player entities and resources.
	///
	class PlayerGameplaySystem 
	{
	public:
		PlayerGameplaySystem( eage::ecs::ECSRegistry& registry, BulletSystem& bullet_system,
							  eage::ecs::AudioSystem& audio_system,
							  eage::ecs::SceneGraphSystem& scene_graph_system );
		~PlayerGameplaySystem();

		void PreparePlayer( eage::ecs::RenderSystem& render_system, uint64_t root_entity,
							assets::SceneResourceLoader& resources );
		
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
		eage::ecs::AudioSystem& mAudioSystem;
		eage::ecs::SceneGraphSystem& mSceneGraphSystem;

		uint32_t mPlayerMaterialId = 0;
		uint32_t mPlayerBulletMaterialId = 0;
		uint32_t mDefaultBulletPoolId = 0;
	};
}

#endif // _VORTEX_PLAYER_GAMEPLAY_SYSTEM_H
