#ifndef _EAGE_RENDERER_H
#define _EAGE_RENDERER_H

#include <memory>
#include <vector>

struct SDL_Window;

namespace graphics
{
	class VulkanContext;
	class VulkanSwapChain;
	class VulkanCommandContext;
	class Renderable;

	///
	/// Renderer class
	///
	class Renderer
	{
	public:
		struct Frame
		{
			std::vector<std::shared_ptr<Renderable>> renderables;
			std::unique_ptr<VulkanCommandContext> command_context;
			uint32_t index = 0;
		};

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

		///
		/// Add a renderable to the render queue
		///
		/// @param renderable
		///  The renderable to add
		///
		void AddToRenderQueue( std::shared_ptr<Renderable> renderable );

	private:
		Frame& GetCurrentFrame();
		
		void Submit();

		void Present();

		std::unique_ptr<VulkanContext>		mContext;
		std::unique_ptr<VulkanSwapChain>	mSwapChain;
		std::vector<Frame>					mFrames;
		int64_t								mFrameNumber;
	};
}

#endif // _EAGE_RENDERER_H