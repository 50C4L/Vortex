#ifndef _EAGE_SCENE_RENDER_PASS_H_
#define _EAGE_SCENE_RENDER_PASS_H_

#include <vector>

#include <graphics/AbstractRenderPass.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/RenderInfo.h>

namespace eage::graphics
{
	class Renderer;

	class SceneRenderPass final : public AbstractRenderPass
	{
	public:
		SceneRenderPass( Renderer& renderer, uint32_t width, uint32_t height );
		~SceneRenderPass() override = default;

		const RenderPassDesc& GetDesc() const override;

		void Execute( vk::CommandBuffer& cmd, const ExecutionContext& ctx ) override;

		void AddRenderInfo( RenderInfo info );

		ManagedImage& GetColorTarget();

	private:
		Renderer& mRenderer;
		ManagedImage::Ptr mColorTarget;
		ManagedImage::Ptr mDepthTarget;
		RenderPassDesc mDesc;
		std::vector<RenderInfo> mRenderQueue;
	};
}

#endif // _EAGE_SCENE_RENDER_PASS_H_
