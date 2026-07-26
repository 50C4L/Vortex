#ifndef _EAGE_PRESENT_PASS_H_
#define _EAGE_PRESENT_PASS_H_

#include <graphics/AbstractRenderPass.h>

namespace eage::graphics
{
	/// Blits the scene output into the swapchain with nearest filtering, or
	/// clears the swapchain black when no source is bound.
	class PresentPass final : public AbstractRenderPass
	{
	public:
		PresentPass();

		void SetSource( ManagedImage* image );

		const RenderPassDesc& GetDesc() const override;

		void Execute( CommandBuffer& cmd, const FrameContext& ctx ) override;

	private:
		ManagedImage*  mSource = nullptr;
		RenderPassDesc mDesc;
	};
}

#endif // _EAGE_PRESENT_PASS_H_
