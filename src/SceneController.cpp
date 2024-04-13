#include "SceneController.h"

#include "AbstractScene.h"

using namespace vortex;

SceneController::SceneController()
	: mCurrentScene( nullptr )
	, mCurrentSceneId( -1 )
{
}

SceneController::~SceneController()
{
	if( mCurrentScene )
	{
		mCurrentScene->OnExit();
	}
}

void
SceneController::AddScene( int64_t id, std::unique_ptr<AbstractScene> scene )
{
	mScenes[id] = std::move( scene );
}

void
SceneController::ChangeScene( int64_t id )
{
	auto it = mScenes.find( id );
	if( it != mScenes.end() )
	{
		if( mCurrentScene )
		{
			mCurrentScene->OnExit();
		}

		mCurrentScene = it->second.get();
		mCurrentSceneId = id;

		mCurrentScene->OnEnter();
	}
}

void
SceneController::Update()
{
	if( mCurrentScene )
	{
		mCurrentScene->Update();
	}
}

void
SceneController::FreeAllScenes()
{
	mScenes.clear();
	mCurrentScene = nullptr;
	mCurrentSceneId = -1;
}
