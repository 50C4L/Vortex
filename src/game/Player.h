#ifndef _VORTEX_PLAYER_H
#define _VORTEX_PLAYER_H

#include <memory>

namespace graphics
{
	class Renderer;
	struct GPUMeshBuffers;
	struct Material;
}

namespace vortex
{
	class Ship;

	class Player
	{
	public:
		Player( graphics::Renderer& renderer );
		virtual ~Player();

		void Init( std::shared_ptr<graphics::GPUMeshBuffers> mesh_buffer, std::shared_ptr<graphics::Material> material );

		void Update();

		void Draw();

	private:
		graphics::Renderer& mRenderer;
		std::unique_ptr<Ship> mShip;
	};
}

#endif