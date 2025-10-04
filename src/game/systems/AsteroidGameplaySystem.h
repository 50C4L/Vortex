#ifndef _VORTEX_ASTEROID_GAMEPLAY_SYSTEM_H_
#define _VORTEX_ASTEROID_GAMEPLAY_SYSTEM_H_

#include <deque>

#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	class ECSRegistry;
	class RenderSystem;
}

namespace eage::graphics
{
	class Renderer;
}

namespace vortex
{
	class AsteroidGameplaySystem
	{
	public:
		AsteroidGameplaySystem( eage::ecs::ECSRegistry& registry, eage::ecs::RenderSystem& render_system,
								eage::graphics::Renderer& renderer );
		~AsteroidGameplaySystem();

		///
		/// Precreate and setup a number of asteroids
		/// Note that this function will perform immediate GPU operations
		///
		void PrepareAsteroids( int count, uint64_t root_entity );

		void Update();

	private:
		eage::ecs::ECSRegistry& mECSRegistry;
		eage::ecs::RenderSystem& mRenderSystem;
		eage::graphics::Renderer& mRenderer;
		eage::ecs::ResourceId mAsteroidMaterialId = 0;
		eage::ecs::ResourceId mAsteroidMeshId = 0;

		std::deque<uint64_t> mAvailableAsteroids;
	};
}

#endif // _VORTEX_ASTEROID_GAMEPLAY_SYSTEM_H_