#ifndef _EAGE_UI_RENDER_PASS_H_
#define _EAGE_UI_RENDER_PASS_H_

#include <graphics/AbstractRenderPass.h>
#include <graphics/ManagedVulkanResources.h>

namespace Rml
{
	class Context;
}

namespace eage::graphics
{
	class Renderer;
}

namespace eage::ui
{
	class UIRenderInterface;

	class UIRenderPass final : public graphics::AbstractRenderPass
	{
	public:
		UIRenderPass(
			graphics::Renderer& renderer,
			UIRenderInterface& render_interface,
			Rml::Context& context,
			uint32_t width,
			uint32_t height );
		~UIRenderPass() override = default;

		void SetGameplayInput( graphics::ManagedImage* image );

		const graphics::RenderPassDesc& GetDesc() const override;
		void Prepare( size_t frame_index ) override;
		void Execute( graphics::CommandBuffer& cmd, const graphics::FrameContext& ctx ) override;

		graphics::ManagedImage* GetColorTarget();

	private:
		graphics::Renderer& mRenderer;
		UIRenderInterface& mRenderInterface;
		Rml::Context& mContext;
		graphics::ManagedImage::Ptr mColorTarget;
		graphics::RenderPassDesc mDesc;
	};
}

#endif // _EAGE_UI_RENDER_PASS_H_
