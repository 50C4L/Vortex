#include "SceneRenderPass.h"

#include <algorithm>
#include <graphics/BuiltInUniforms.h>
#include <graphics/Material.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/Renderer.h>
#include <graphics/VMAWrapper.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>

using namespace eage::graphics;

namespace
{
	vk::RenderingAttachmentInfo create_attachment_info( vk::ImageView view, std::optional<vk::ClearValue> clear, vk::ImageLayout layout )
	{
		vk::RenderingAttachmentInfo attachment_info{};
		attachment_info.imageView = view;
		attachment_info.imageLayout = layout;
		attachment_info.loadOp = clear.has_value() ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
		if( clear.has_value() )
		{
			attachment_info.clearValue = clear.value();
		}

		return attachment_info;
	}
}

SceneRenderPass::SceneRenderPass( Renderer& renderer, uint32_t width, uint32_t height )
	: mRenderer( renderer )
	, mColorTarget( ManagedImage::Create(
		renderer.GetDevice(),
		*renderer.GetMemoryAllocator().allocator.get(),
		vk::Extent3D{ width, height, 1 },
		renderer.GetColorFormat(),
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
		vk::ImageAspectFlagBits::eColor ) )
	, mDepthTarget( ManagedImage::Create(
		renderer.GetDevice(),
		*renderer.GetMemoryAllocator().allocator.get(),
		vk::Extent3D{ width, height, 1 },
		renderer.GetDepthFormat(),
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::ImageAspectFlagBits::eDepth ) )
{
	mDesc.color_target = mColorTarget.get();
	mDesc.depth_target = mDepthTarget.get();
	mDesc.clear_color = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
	mDesc.clear_depth = 1.f;
}

const RenderPassDesc&
SceneRenderPass::GetDesc() const
{
	return mDesc;
}

void
SceneRenderPass::Execute( CommandBuffer& buffer, const FrameContext& ctx )
{
	vk::CommandBuffer cmd( static_cast<VkCommandBuffer>( buffer.GetNativeHandle() ) );
	std::optional<vk::ClearValue> color_clear;
	if( mDesc.clear_color.has_value() )
	{
		auto c = mDesc.clear_color.value();
		color_clear = vk::ClearColorValue{ std::array<float,4>{ c.r, c.g, c.b, c.a } };
	}
	auto color_attachment = create_attachment_info( mColorTarget->image_view.get(), color_clear, vk::ImageLayout::eColorAttachmentOptimal );

	std::optional<vk::ClearValue> depth_clear;
	if( mDesc.clear_depth.has_value() )
	{
		vk::ClearValue dv;
		dv.depthStencil.depth = mDesc.clear_depth.value();
		depth_clear = dv;
	}
	auto depth_attachment = create_attachment_info( mDepthTarget->image_view.get(), depth_clear, vk::ImageLayout::eDepthAttachmentOptimal );

	vk::Extent2D render_extent = { mColorTarget->extent.width, mColorTarget->extent.height };
	vk::RenderingInfo rendering_info{};
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &color_attachment;
	rendering_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, std::move( render_extent ) };
	rendering_info.layerCount = 1;
	rendering_info.pDepthAttachment = &depth_attachment;

	cmd.beginRendering( rendering_info );

	// Sort back-to-front by world Z for correct alpha blending
	std::sort( mRenderQueue.begin(), mRenderQueue.end(),
		[]( const RenderInfo& a, const RenderInfo& b )
		{
			return a.model_matrix[3][2] < b.model_matrix[3][2];
		} );

	size_t current_frame = ctx.frame_index;
	for( auto& render_info : mRenderQueue )
	{
		cmd.bindPipeline( vk::PipelineBindPoint::eGraphics, render_info.material->pipeline->pipeline.get() );

		vk::Viewport viewport{};
		viewport.width = static_cast<float>( render_extent.width );
		viewport.height = static_cast<float>( render_extent.height );
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.x = 0;
		viewport.y = 0;
		cmd.setViewport( 0, viewport );

		vk::Rect2D scissor{};
		scissor.extent = render_extent;
		scissor.offset = vk::Offset2D{ 0, 0 };
		cmd.setScissor( 0, scissor );

		MeshUniformData data;
		data.model = render_info.model_matrix;
		data.vertex_buffer_address = render_info.mesh_buffer->vertex_buffer_address;
		data.uv_rect = render_info.uv_rect;
		render_info.mesh_uniform_data_dynamic->Update( &data, sizeof( MeshUniformData ), sizeof( MeshUniformData ) * current_frame );

		// Pipeline global uniform
		uint32_t descriptor_index = 0;
		{
			auto dynamic_offsets = render_info.material->pipeline->global_descriptor->GetDynamicOffsets( current_frame );
			cmd.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				render_info.material->pipeline->layout.get(),
				descriptor_index++, 1,
				render_info.material->pipeline->global_descriptor->GetDescriptorSet( current_frame ),
				static_cast<uint32_t>( dynamic_offsets->size() ), dynamic_offsets->data() );
		}

		// Per-object predefined uniform
		{
			auto dynamic_offsets = render_info.mesh_descriptor->GetDynamicOffsets( current_frame );
			cmd.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				render_info.material->pipeline->layout.get(),
				descriptor_index++, 1,
				render_info.mesh_descriptor->GetDescriptorSet( current_frame ),
				static_cast<uint32_t>( dynamic_offsets->size() ), dynamic_offsets->data() );
		}

		// Material static uniform
		{
			cmd.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				render_info.material->pipeline->layout.get(),
				descriptor_index++, 1,
				render_info.material->descriptor->GetDescriptorSet(),
				0, nullptr );
		}

		cmd.bindIndexBuffer( render_info.mesh_buffer->index_buffer->buffer, 0, vk::IndexType::eUint32 );
		cmd.drawIndexed( render_info.index_count, 1, render_info.first_index, render_info.vertex_offset, 0 );
	}

	cmd.endRendering();

	mRenderQueue.clear();
}

void
SceneRenderPass::AddRenderInfo( RenderInfo info )
{
	mRenderQueue.push_back( std::move( info ) );
}

ManagedImage&
SceneRenderPass::GetColorTarget()
{
	return *mColorTarget;
}
