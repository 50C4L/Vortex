#ifndef _ANIMTOOL_APP_H_
#define _ANIMTOOL_APP_H_

#include <memory>
#include <optional>

#include <glm/glm.hpp>

struct SDL_Window;

namespace eage::ecs
{
	class ECSRegistry;
	class RenderSystem;
}

namespace eage::graphics
{
	class Renderer;
	class SceneRenderPass;
	class ImGuiRenderPass;
	class OrthographicCamera;
}

namespace animtool
{

class FileDialog;
class FrameSequence;

struct ExportDialogState
{
	bool show_modal = false;
	char animation_name[128] = "animation";
};

struct PreviewPlaybackState
{
	bool playing = false;
	size_t current_frame = 0;
	float elapsed_sec = 0.f;

	void Start();
	void Stop();
	void Update( float delta_time_sec, const FrameSequence& frame_sequence );
};

class AnimToolApp
{
public:
	AnimToolApp();
	~AnimToolApp();

	bool Init();
	void Run();

private:
	void InitPreviewRendering();
	void UpdatePreviewPlayback( float delta_time_sec );
	void UpdatePreviewSprite();
	void DrawToolUI();

	std::shared_ptr<SDL_Window> mWindow;
	std::unique_ptr<FileDialog> mFileDialog;
	std::unique_ptr<eage::graphics::Renderer> mRenderer;
	std::unique_ptr<eage::ecs::ECSRegistry> mECSRegistry;
	std::unique_ptr<eage::ecs::RenderSystem> mRenderSystem;
	std::unique_ptr<eage::graphics::SceneRenderPass> mScenePass;
	std::unique_ptr<eage::graphics::ImGuiRenderPass> mImGuiPass;
	std::unique_ptr<eage::graphics::OrthographicCamera> mPreviewCamera;
	std::unique_ptr<FrameSequence> mFrameSequence;
	ExportDialogState mExportState;
	PreviewPlaybackState mPreviewPlayback;
	bool mPlayRequested = false;

	uint64_t mPreviewEntity = 0;
	uint32_t mPreviewMaterialId = 0;
	uint32_t mPreviewMeshId = 0;
	int mPreviewMeshWidth = 0;
	int mPreviewMeshHeight = 0;
	glm::vec2 mPreviewVirtualResolution{ 1.f, 1.f };
};

}

#endif // _ANIMTOOL_APP_H_
