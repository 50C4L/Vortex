#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "../AbstractScene.h"

#include <memory>

namespace graphics
{
	class Renderable;;
	class Renderer;
	class OrthographicCamera;
}

namespace vortex
{
	class MainScene : public AbstractScene
	{
	public:
		MainScene( graphics::Renderer& renderer );
		virtual ~MainScene();

		virtual void OnEnter() override;
		virtual void OnExit() override;

		virtual void Update() override;

	private:
		graphics::Renderer& mRenderer;
		std::shared_ptr<graphics::Renderable> mPlayer;
		std::shared_ptr<graphics::OrthographicCamera> mCamera;
	};
}

#endif // _MAIN_SCENE_H