#include "CompositePass.h"

#include <array>
#include <optional>

#include <graphics/Renderer.h>
#include <graphics/VMAWrapper.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <utility/Logger.h>

using namespace eage::graphics;
using namespace utility;

namespace
{
	constexpr const char* COMPOSITE_VERT_PATH = "./src/core/graphics/shaders/compiled/composite.vert.spv";
	constexpr const char* COMPOSITE_FRAG_PATH = "./src/core/graphics/shaders/compiled/composite.frag.spv";

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

CompositePass::CompositePass( Renderer& renderer, uint32_t width, uint32_t height )
	: mRenderer( renderer )
	, mColorTarget( ManagedImage::Create(
		renderer.GetDevice(),
		*renderer.GetMemoryAllocator().allocator.get(),
		vk::Extent3D{ width, height, 1 },
		renderer.GetColorFormat(),
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
		vk::ImageAspectFlagBits::eColor ) )
{
	mDesc.color_target = mColorTarget.get();
	mDesc.clear_color = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
}

void
CompositePass::SetInputs( ManagedImage* base, ManagedImage* overlay )
{
	mBase = base;
	mOverlay = overlay;
	mDesc.input_images.clear();

	if( mBase )
	{
		mDesc.input_images.push_back( PassInput{ mBase, ImageAccess::ShaderRead } );
		mBaseBindlessIndex = mRenderer.RegisterBindlessTexture(
			mBase->image_view.get(),
			mRenderer.GetDefaultSampler() );
	}

	if( mOverlay )
	{
		mDesc.input_images.push_back( PassInput{ mOverlay, ImageAccess::ShaderRead } );
		mOverlayBindlessIndex = mRenderer.RegisterBindlessTexture(
			mOverlay->image_view.get(),
			mRenderer.GetDefaultSampler() );
	}
}

const RenderPassDesc&
CompositePass::GetDesc() const
{
	return mDesc;
}

bool
CompositePass::EnsurePipeline()
{
	if( mPipelineReady )
	{
		return true;
	}

	auto vert = load_shader_from_file( mRenderer.GetDevice(), COMPOSITE_VERT_PATH );
	auto frag = load_shader_from_file( mRenderer.GetDevice(), COMPOSITE_FRAG_PATH );
	if( !vert || !frag )
	{
		LOG_ERROR( "CompositePass: failed to load composite shaders" );
		return false;
	}

	mVertModule = std::move( vert->module );
	mFragModule = std::move( frag->module );

	vk::PushConstantRange push_range{};
	push_range.stageFlags = vk::ShaderStageFlagBits::eFragment;
	push_range.offset = 0;
	push_range.size = sizeof( PushConstants );

	vk::DescriptorSetLayout set_layouts[] = {
		mRenderer.GetBuiltInDescriptorSetLayouts().bindless.get()
	};

	vk::PipelineLayoutCreateInfo layout_info{};
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = set_layouts;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges = &push_range;
	mPipelineLayout = mRenderer.GetDevice().createPipelineLayoutUnique( layout_info );

	VulkanPipelineBuilder builder;
	mPipeline = builder
		.SetShaders( mVertModule.get(), mFragModule.get() )
		.SetPipelineLayout( mPipelineLayout.get() )
		.SetInputTopology( vk::PrimitiveTopology::eTriangleList )
		.SetPolygonMode( vk::PolygonMode::eFill )
		.SetCullMode( vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise )
		.SetMultisampling()
		.SetBlendMode(
			VK_FALSE,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero )
		.SetDepthTest( VK_FALSE, VK_FALSE, vk::CompareOp::eAlways )
		.SetColorAttachmentFormat( mRenderer.GetColorFormat() )
		.Build( mRenderer.GetDevice() );

	if( !mPipeline )
	{
		LOG_ERROR( "CompositePass: failed to create pipeline" );
		return false;
	}

	mPipelineReady = true;
	return true;
}

void
CompositePass::Execute( CommandBuffer& buffer, const FrameContext& /*ctx*/ )
{
	if( !mBase || !mOverlay )
	{
		LOG_ERROR( "CompositePass: missing base or overlay input" );
		return;
	}

	if( !EnsurePipeline() )
	{
		return;
	}

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

	vk::Viewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>( render_extent.width );
	viewport.height = static_cast<float>( render_extent.height );
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	cmd.setViewport( 0, 1, &viewport );

	vk::Rect2D scissor{ vk::Offset2D{ 0, 0 }, render_extent };
	cmd.setScissor( 0, 1, &scissor );

	cmd.bindPipeline( vk::PipelineBindPoint::eGraphics, mPipeline.get() );

	vk::DescriptorSet bindless_set = mRenderer.GetBindlessDescriptorSet();
	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		mPipelineLayout.get(),
		0,
		1,
		&bindless_set,
		0,
		nullptr );

	PushConstants push{};
	push.base_index = mBaseBindlessIndex;
	push.overlay_index = mOverlayBindlessIndex;
	cmd.pushConstants(
		mPipelineLayout.get(),
		vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof( PushConstants ),
		&push );

	cmd.draw( 3, 1, 0, 0 );

	cmd.endRendering();
}

ManagedImage*
CompositePass::GetColorTarget()
{
	return mColorTarget.get();
}
