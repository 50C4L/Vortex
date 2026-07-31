#ifndef _EAGE_COMPOSITE_PASS_H_
#define _EAGE_COMPOSITE_PASS_H_

#include <cstdint>

#include <vulkan/vulkan.hpp>

#include <graphics/AbstractRenderPass.h>
#include <graphics/ManagedVulkanResources.h>

namespace eage::graphics
{
	class Renderer;

	/// Fullscreen alpha composite of two arbitrary images into an owned color target.
	/// Result: overlay over base (straight alpha).
	class CompositePass final : public AbstractRenderPass
	{
	public:
		CompositePass( Renderer& renderer, uint32_t width, uint32_t height );
		~CompositePass() override = default;

		void SetInputs( ManagedImage* base, ManagedImage* overlay );

		const RenderPassDesc& GetDesc() const override;
		void Execute( CommandBuffer& cmd, const FrameContext& ctx ) override;

		ManagedImage* GetColorTarget();

	private:
		struct PushConstants
		{
			uint32_t base_index = 0;
			uint32_t overlay_index = 0;
		};

		bool EnsurePipeline();

		Renderer& mRenderer;
		ManagedImage::Ptr mColorTarget;
		RenderPassDesc mDesc;

		ManagedImage* mBase = nullptr;
		ManagedImage* mOverlay = nullptr;
		uint32_t mBaseBindlessIndex = 0;
		uint32_t mOverlayBindlessIndex = 0;

		vk::UniqueShaderModule mVertModule;
		vk::UniqueShaderModule mFragModule;
		vk::UniquePipelineLayout mPipelineLayout;
		vk::UniquePipeline mPipeline;
		bool mPipelineReady = false;
	};
}

#endif // _EAGE_COMPOSITE_PASS_H_
