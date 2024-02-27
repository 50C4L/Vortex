#ifndef _ABSTRACT_SCENE_H
#define _ABSTRACT_SCENE_H

namespace vortex
{
	class AbstractScene
	{
	public:
		AbstractScene() {}
		virtual ~AbstractScene() {}

		virtual void OnEnter() = 0;
		virtual void Update() = 0;
		virtual void OnExit() = 0;
	};
}

#endif // _ABSTRACT_SCENE_H