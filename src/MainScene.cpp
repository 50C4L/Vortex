#include "MainScene.h"

#include <utility/Logger.h>

using namespace vortex;
using namespace utility;

MainScene::MainScene()
{
}

MainScene::~MainScene()
{
}

void
MainScene::OnEnter()
{
	LOG( "MainScene::OnEnter" );
}

void
MainScene::OnExit()
{
	LOG( "MainScene::OnExit" );
}

void
MainScene::Update()
{
	// player input

	// player update

	// enemy update

	// collision detection

	// mRenderer->AddToRenderQueue( mPlayer );
}