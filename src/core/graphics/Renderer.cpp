#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>

#include <utility/Logger.h>
#include <graphics/VulkanContext.h>
#include <graphics/VulkanSwapChain.h>

using namespace graphics;
using namespace utility;


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
}
