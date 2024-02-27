#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "AbstractScene.h"

namespace vortex
{
	class MainScene : public AbstractScene
	{
	public:
		MainScene();
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual void OnExit() override;

		virtual void Update() override;
	};
}

#endif // _MAIN_SCENE_H