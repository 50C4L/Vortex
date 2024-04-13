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

		void AddScene( int64_t id, std::unique_ptr<AbstractScene> scene );

		void ChangeScene( int64_t id );

		void Update();

		void FreeAllScenes();

	private:
		std::unordered_map<int64_t, std::unique_ptr<AbstractScene>> mScenes;
		AbstractScene* mCurrentScene;
		int64_t mCurrentSceneId;
	};
}

#endif // _SCENE_CONTROLLER_H