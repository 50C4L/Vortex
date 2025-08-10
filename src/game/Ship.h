#ifndef _VORTEX_SHIP_H
#define _VORTEX_SHIP_H

#include <memory>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <graphics/Material.h>

namespace graphics
{
	class Renderer;
	class RenderComponent;
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

		void SetBodyMaterial( std::unique_ptr<graphics::Material> material );
		void SetThrustMaterial( std::unique_ptr<graphics::Material> material );
		void Update( glm::mat4 transform );
		void Draw( bool is_thrust_on );

		void SetRotateSpeed( float angle );
		void SetMaxThrustSpeed( float speed );
		void SetThrustAcceleration( float acceleration );

		void Thrust( bool on );

	private:
		graphics::Renderer& mRenderer;
		float mRotateSpeed;
		float mMaxThrustSpeed;
		float mThrustAcceleration;
		glm::vec3 mVelocity;
		glm::vec3 mForwardDir;
		glm::vec3 mPosition;

		// Render components
		size_t mShipBodyRCIndex = -1;
		size_t mThrustRCIndex = -1;
		std::vector<std::unique_ptr<graphics::RenderComponent>> mRenderComponents;
	};
}

#endif // _VORTEX_SHIP_H