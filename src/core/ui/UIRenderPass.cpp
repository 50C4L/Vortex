#include "UIRenderPass.h"

#include <array>
#include <optional>

#include <RmlUi/Core/Context.h>

#include <graphics/Renderer.h>
#include <graphics/VMAWrapper.h>
#include <ui/UIRenderInterface.h>

using namespace eage::ui;
using namespace eage::graphics;

namespace
{
	vk::RenderingAttachmentInfo create_color_attachment( vk::ImageView view, std::optional<vk::ClearValue> clear )
	{
		vk::RenderingAttachmentInfo attachment{};
		attachment.imageView = view;
		attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		attachment.loadOp = clear.has_value() ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		attachment.storeOp = vk::AttachmentStoreOp::eStore;
		if( clear.has_value() )
		{
			attachment.clearValue = clear.value();
		}
		return attachment;
	}
}

UIRenderPass::UIRenderPass(
	Renderer& renderer,
	UIRenderInterface& render_interface,
	Rml::Context& context,
	uint32_t width,
	uint32_t height )
	: mRenderer( renderer )
	, mRenderInterface( render_interface )
	, mContext( context )
	, mColorTarget( ManagedImage::Create(
		renderer.GetDevice(),
		*renderer.GetMemoryAllocator().allocator.get(),
		vk::Extent3D{ width, height, 1 },
		renderer.GetColorFormat(),
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
		vk::ImageAspectFlagBits::eColor ) )
{
	mDesc.color_target = mColorTarget.get();
	mDesc.clear_color = glm::vec4{ 0.f, 0.f, 0.f, 0.f };
}

const RenderPassDesc&
UIRenderPass::GetDesc() const
{
	return mDesc;
}

void
UIRenderPass::Prepare( size_t /*frame_index*/ )
{
	mRenderInterface.AdvanceFrame();
	mContext.Update();
}

void
UIRenderPass::Execute( CommandBuffer& buffer, const FrameContext& /*ctx*/ )
{
	vk::CommandBuffer cmd( static_cast<VkCommandBuffer>( buffer.GetNativeHandle() ) );

	std::optional<vk::ClearValue> color_clear;
	if( mDesc.clear_color.has_value() )
	{
		const auto c = mDesc.clear_color.value();
		color_clear = vk::ClearColorValue{ std::array<float, 4>{ c.r, c.g, c.b, c.a } };
	}

	auto color_attachment = create_color_attachment( mColorTarget->image_view.get(), color_clear );

	vk::Extent2D render_extent{ mColorTarget->extent.width, mColorTarget->extent.height };
	vk::RenderingInfo rendering_info{};
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &color_attachment;
	rendering_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, render_extent };
	rendering_info.layerCount = 1;

	cmd.beginRendering( rendering_info );

	mRenderInterface.BeginFrame( buffer, render_extent.width, render_extent.height );
	mContext.Render();
	mRenderInterface.EndFrame();

	cmd.endRendering();
}

ManagedImage*
UIRenderPass::GetColorTarget()
{
	return mColorTarget.get();
}
