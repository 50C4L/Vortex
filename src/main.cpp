#define SDL_MAIN_HANDLED
#include <iostream>

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

int main() {
	SDL_Window *window = nullptr;
	VkInstance instance = nullptr;

	// Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		return 1;
	}

	// Create SDL window
	window = SDL_CreateWindow( "Vortex Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN );
	if( window == nullptr )
	{
		std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	// Initialize Vulkan
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName = "Vulkan Engine";
	appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if( vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS )
	{
		std::cerr << "Failed to create Vulkan instance" << std::endl;
		SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	// Main loop
	bool quit = false;
	while( !quit )
	{
		SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
			{
				quit = true;
			}
		}
	}

	// Cleanup
	vkDestroyInstance( instance, nullptr );
	SDL_DestroyWindow( window );
	SDL_Quit();
	
	return 0;
}