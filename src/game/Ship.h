#ifndef _VORTEX_SHIP_H
#define _VORTEX_SHIP_H

#include <memory>

#include <vulkan/vulkan.hpp>

namespace graphics
{
	class Renderer;
	class RenderComponent;
	struct ManagedBuffer;
	struct Material;
	class UniformDescriptor;
}

namespace vortex
{
	/// 
	/// Defines a ship
	/// A ship has thruster on the rear, can only accelerate forward, and can rotate
	///
	class Ship
	{
	public:
		///
		/// Constructor
		///
		Ship( graphics::Renderer& renderer );

		///
		/// Destructor
		///
		virtual ~Ship();

		std::shared_ptr<graphics::RenderComponent> GetRenderComponent() const;

		void Update( float delta_time );

		void SetRotateSpeed( float angle );

	private:
		std::shared_ptr<graphics::RenderComponent> mRenderComponent;
		float mRotateSpeed;
	};
}

#endif // _VORTEX_SHIP_H