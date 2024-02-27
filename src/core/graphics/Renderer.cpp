#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>

#include <utility/Logger.h>
#include <graphics/VulkanContext.h>
#include <graphics/VulkanSwapChain.h>

using namespace graphics;
using namespace utility;

namespace
{
	const int MAX_FRAMES_IN_FLIGHT = 2; // Double buffering
}


Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

bool
Renderer::Init( SDL_Window& window )
{
	LOG( "Initializing vulkan context ..." );
	mContext = std::make_unique<VulkanContext>( window );

	LOG( "Initializing vulkan swap chain ..." );
	mSwapChain = std::make_unique<VulkanSwapChain>( *mContext, 800, 600 );

	return true;
}

void
Renderer::Render()
{
	/* TODO
	vulkan_main_buffer->begin();
	vulkan_main_buffer->clear( 0.0f, 0.0f, 0.0f, 1.0f );

	for( const auto& renderable : GetCurrentFrame().mRenderables )
	{
		renderable.Render( *vulkan_main_buffer );
	}

	vulkan_main_buffer->end();
	*/
}

/*TODO
void
Renderer::AddToRenderQueue( const Renderable& renderable )
{
	GetCurrentFrame().mRenderables.push_back( renderable );
}
*/
