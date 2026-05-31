#include "SceneController.h"

#include <algorithm>

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
SceneController::AddScene( int id, std::unique_ptr<AbstractScene> scene )
{
	mScenes[id] = std::move( scene );
}

void
SceneController::ChangeScene( int id )
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
		NotifyObservers( mCurrentScene->GetSceneRoot() );
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

uint64_t
SceneController::GetCurrentSceneRoot()
{
	if( mCurrentScene )
	{
		return mCurrentScene->GetSceneRoot();
	}
	return 0;
}

void
SceneController::Subscribe( Observer* observer )
{
	mObservers.push_back( observer );
}

void
SceneController::Unsubscribe( Observer* observer )
{
	mObservers.erase(
		std::remove( mObservers.begin(), mObservers.end(), observer ),
		mObservers.end() );
}

void
SceneController::NotifyObservers( uint64_t scene_root )
{
	for( auto* observer : mObservers )
	{
		observer->OnSceneChanged( scene_root );
	}
}
