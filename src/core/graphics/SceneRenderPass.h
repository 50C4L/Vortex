#ifndef _EAGE_SCENE_RENDER_PASS_H_
#define _EAGE_SCENE_RENDER_PASS_H_

#include <vector>

#include <graphics/AbstractRenderPass.h>
#include <graphics/RenderInfo.h>

namespace eage::graphics
{
	class Renderer;

	class SceneRenderPass final : public AbstractRenderPass
	{
	public:
		SceneRenderPass( Renderer& renderer, ManagedImage& color_target, ManagedImage& depth_target );
		~SceneRenderPass() override = default;

		const RenderPassDesc& GetDesc() const override;

		void Execute( vk::CommandBuffer& cmd, const ExecutionContext& ctx ) override;

		void AddRenderInfo( RenderInfo info );

	private:
		Renderer& mRenderer;
		ManagedImage& mColorTarget;
		ManagedImage& mDepthTarget;
		RenderPassDesc mDesc;
		std::vector<RenderInfo> mRenderQueue;
	};
}

#endif // _EAGE_SCENE_RENDER_PASS_H_
