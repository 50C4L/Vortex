#ifndef _SCENE_CONTROLLER_H
#define _SCENE_CONTROLLER_H

#include <memory>
#include <unordered_map>
#include <vector>

namespace vortex
{
	class AbstractScene;

	///
	/// SceneController: Manages scene transitions and updates the current scene
	///
	class SceneController
	{
	public:
		///
		/// Observer interface notified whenever the active scene changes.
		///
		class Observer
		{
		public:
			virtual ~Observer() = default;
			virtual void OnSceneChanged( uint64_t scene_root ) = 0;
		};

		SceneController();
		virtual ~SceneController();

		void AddScene( int id, std::unique_ptr<AbstractScene> scene );

		void ChangeScene( int id );

		void Update( float dt );

		void FreeAllScenes();

		uint64_t GetCurrentSceneRoot();

		AbstractScene* GetCurrentScene() const;

		void Subscribe( Observer* observer );
		void Unsubscribe( Observer* observer );

	private:
		void NotifyObservers( uint64_t scene_root );

		std::unordered_map<int, std::unique_ptr<AbstractScene>> mScenes;
		AbstractScene* mCurrentScene;
		int mCurrentSceneId;
		std::vector<Observer*> mObservers;
	};
}

#endif // _SCENE_CONTROLLER_H