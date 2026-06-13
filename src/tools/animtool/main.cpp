#define SDL_MAIN_HANDLED

#include "AnimToolApp.h"

int
main()
{
	animtool::AnimToolApp app;

	if( !app.Init() )
	{
		return 1;
	}

	app.Run();

	return 0;
}
