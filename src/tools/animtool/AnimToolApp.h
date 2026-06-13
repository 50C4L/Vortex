#ifndef _ANIMTOOL_APP_H_
#define _ANIMTOOL_APP_H_

#include <memory>

struct SDL_Window;

namespace eage::graphics
{
	class Renderer;
	class SceneRenderPass;
	class ImGuiRenderPass;
}

namespace animtool
{

class AnimToolApp
{
public:
	AnimToolApp();
	~AnimToolApp();

	bool Init();
	void Run();

private:
	std::shared_ptr<SDL_Window> mWindow;
	std::unique_ptr<eage::graphics::Renderer> mRenderer;
	std::unique_ptr<eage::graphics::SceneRenderPass> mScenePass;
	std::unique_ptr<eage::graphics::ImGuiRenderPass> mImGuiPass;
};

}

#endif // _ANIMTOOL_APP_H_
