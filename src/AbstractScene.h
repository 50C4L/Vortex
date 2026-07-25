#ifndef _ABSTRACT_SCENE_H
#define _ABSTRACT_SCENE_H

#include <cstdint>

namespace eage::graphics
{
	struct ManagedImage;
}

namespace vortex
{
	class AbstractScene
	{
	public:
		AbstractScene() {}
		virtual ~AbstractScene() {}

		virtual void OnEnter() = 0;
		virtual uint64_t GetSceneRoot() = 0;
		virtual void Update( float dt ) = 0;
		virtual void OnExit() = 0;

		/// Image the shell should present. nullptr if the scene has no output.
		virtual eage::graphics::ManagedImage* GetOutput() { return nullptr; }
	};
}

#endif // _ABSTRACT_SCENE_H
