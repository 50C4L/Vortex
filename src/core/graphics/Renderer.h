#ifndef _EAGE_RENDERER_H
#define _EAGE_RENDERER_H

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace graphics
{
	///
	/// Renderer class
	///
	class Renderer
	{
	public:
		///
		/// Constructor
		///
		Renderer();

		///
		/// Destructor
		///
		virtual ~Renderer();

		///
		/// Initialize the renderer
		///
		///	@param window
		///	 The window to render to
		///
		/// @return
		///  true if successful, false otherwise
		///
		bool Init( SDL_Window& window );

		///
		/// Output the rendered frame
		///
		void Render();

	private:
		vk::UniqueInstance mInstance;
	};
}

#endif // _EAGE_RENDERER_H