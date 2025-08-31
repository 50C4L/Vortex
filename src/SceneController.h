#ifndef _SCENE_CONTROLLER_H
#define _SCENE_CONTROLLER_H

#include <memory>
#include <unordered_map>

namespace vortex
{
	class AbstractScene;

	class SceneController
	{
	public:
		SceneController();
		virtual ~SceneController();

		void AddScene( int id, std::unique_ptr<AbstractScene> scene );

		void ChangeScene( int id );

		void Update();

		void FreeAllScenes();

		uint64_t GetCurrentSceneRoot();

	private:
		std::unordered_map<int, std::unique_ptr<AbstractScene>> mScenes;
		AbstractScene* mCurrentScene;
		int mCurrentSceneId;
	};
}

#endif // _SCENE_CONTROLLER_H