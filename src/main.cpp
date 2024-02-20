#define SDL_MAIN_HANDLED

#include "VortexGame.h"

int 
main()
{
	vortex::VortexGame game;

	if( !game.Init() )
	{
		return 1;
	}

	game.Run();

	return 0;
}