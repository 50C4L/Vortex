#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_system.h>
#include <iostream>

#include <utility/Logger.h>
#include <graphics/ImageUtilities.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanContext.h>
#include <graphics/VulkanSwapChain.h>
#include <graphics/VulkanShader.h>
#include <graphics/VulkanCommandContext.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VMAWrapper.h>
#include <graphics/ImGUILifetime.h>
#include <graphics/Renderable.h>
#include <graphics/Camera.h>

#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_sdl2.h>

using namespace graphics;
using namespace utility;

namespace
{
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2u; // Double buffering
	const uint32_t DEFAULT_DESCRIPTOR_SET_COUNT = 1000u;

	vk::SubmitInfo2 create_submit_info( vk::CommandBufferSubmitInfo cmd_submit_info, std::optional<vk::SemaphoreSubmitInfo> semaphore_wait_info, std::optional<vk::SemaphoreSubmitInfo> semaphore_signal_info )
	{
		vk::SubmitInfo2 submit_info{};
		submit_info.waitSemaphoreInfoCount = semaphore_wait_info.has_value() ? 1 : 0;
		submit_info.pWaitSemaphoreInfos = semaphore_wait_info.has_value() ? &semaphore_wait_info.value() : nullptr;
		submit_info.signalSemaphoreInfoCount = semaphore_signal_info.has_value() ? 1 : 0;
		submit_info.pSignalSemaphoreInfos = semaphore_signal_info.has_value() ? &semaphore_signal_info.value() : nullptr;
		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &cmd_submit_info;

		return submit_info;
	}

	vk::RenderingAttachmentInfo create_attachment_info( vk::ImageView& view, std::optional<vk::ClearValue> clear, vk::ImageLayout layout )
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

	void render_imgui( vk::CommandBuffer& cmd, vk::ImageView& target_image_view, vk::Extent2D extent )
	{
		vk::RenderingAttachmentInfo color_attachment_info = create_attachment_info( target_image_view, std::nullopt, vk::ImageLayout::eGeneral );
		vk::RenderingInfo render_info{};
		render_info.colorAttachmentCount = 1;
		render_info.pColorAttachments = &color_attachment_info;
		render_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, std::move( extent ) };
		render_info.layerCount = 1;

		cmd.beginRendering( render_info );
		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
		cmd.endRendering();
	}
}


Renderer::Renderer( SDL_Window& window )
	: mWindow( window )
	, mFrameNumber( 0 )
{
}

Renderer::~Renderer()
{
}

bool
Renderer::Init()
{
	LOG( "Initializing vulkan context ..." );
	mContext = std::make_unique<VulkanContext>( mWindow );

	int width, height = 0;
	SDL_Vulkan_GetDrawableSize( &mWindow, &width, &height );

	LOG( "Initializing vulkan swap chain ..." );
	mSwapChain = std::make_unique<VulkanSwapChain>( *mContext, static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) );
	if( mSwapChain->GetImages().size() <MAX_FRAMES_IN_FLIGHT )
	{
		LOG_ERROR( "Swap chain does not have enough images." );
		return false;
	}

	LOG( "Initializing VMA ..." );
	mVMA = std::make_unique<VMAWrapper>( *mContext );

	InitFrameResources();
	mImmidiateCommandContext = std::make_unique<VulkanCommandContext>( *mContext );

	LOG( "Creating render image ..." );

	mRenderImage = std::make_unique<ManagedImage>(
		mContext->logical_device.get(),
		*mVMA->allocator.get(),
		vk::Extent3D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 },
		vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment,
		vk::ImageAspectFlagBits::eColor
	);

	LOG( "Creating depth image ..." );

	mDepthImage = std::make_unique<ManagedImage>(
		mContext->logical_device.get(),
		*mVMA->allocator.get(),
		vk::Extent3D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 },
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::ImageAspectFlagBits::eDepth
	);

	// Init descriptor set layout
	InitDescriptors();

	// Init pipelines
	InitPipelines();

	// Init IMGUI
	InitImGUI();

	return true;
}

void
Renderer::Render()
{
	PrepareImGUI();

	auto& frame = GetCurrentFrame();
	auto& cmd = frame.command_context->GetPrimaryBuffer();
	uint32_t next_image_index = mSwapChain->GetNextImage( frame.command_context->GetSwapchainSemaphore() );

	frame.command_context->WaitForCompletion();
	frame.command_context->Reset();
	frame.command_context->Begin();

	// Transition the main render image to a general layout
	transition_image( cmd, mRenderImage->GetImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral );
	cmd.clearColorImage( 
		mRenderImage->GetImage(),
		vk::ImageLayout::eGeneral,
		vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 1.0f } },
		vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );

	transition_image( cmd, mRenderImage->GetImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal );
	transition_image( cmd, mDepthImage->GetImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal );

	// Actual rendering here
	DrawRenderables( cmd );

	auto render_extent = mRenderImage->GetExtent2D();

	// End of rendering

	// Transition the main render image and the current swapchain image to the appropriate layout, so later we can copy the render image to the swapchain image
	transition_image( cmd, mRenderImage->GetImage(), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal );
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal );

	// Copy the render image to the swapchain image
	copy_image_to_image( cmd, mRenderImage->GetImage(), mSwapChain->GetImages()[ next_image_index ], render_extent, mSwapChain->GetExtent() );

	// Transition the swapchain image to a color optimal layout so we can draw imgui on it
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal );

	// Render imgui
	render_imgui( cmd, mSwapChain->GetImageViews()[ next_image_index ].get(), mSwapChain->GetExtent() );

	// Transition the swapchain image back to the present layout
	transition_image( cmd, mSwapChain->GetImages()[ next_image_index ], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR );

	frame.command_context->End();

	Submit();

	Present( next_image_index );

	mFrameNumber++;
}

void
Renderer::AddToRenderQueue( std::shared_ptr<Renderable> renderable )
{
	if( mFrames.empty() )
	{
		LOG_ERROR( "No frames available, Init() must be called first." );
		return;
	}
	mRenderQueue.push_back( std::move( renderable ) );
}

void
Renderer::WaitForIdle()
{
	mContext->logical_device->waitIdle();
}

std::shared_ptr<GPUMeshBuffers>
Renderer::UploadMesh( std::span<uint32_t> indices, std::span<Vertex> vertices )
{
	const size_t vertex_buffer_size = sizeof( Vertex ) * vertices.size();
	const size_t index_buffer_size = sizeof( uint32_t ) * indices.size();

	GPUMeshBuffers new_surface{
		ManagedBuffer::Create( *mVMA->allocator.get(), index_buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY ),
		ManagedBuffer::Create( *mVMA->allocator.get(), vertex_buffer_size, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY )
	};

	vk::BufferDeviceAddressInfo vertex_buffer_address_info{};
	vertex_buffer_address_info.buffer = new_surface.vertex_buffer->buffer;
	new_surface.vertex_buffer_address = mContext->logical_device->getBufferAddress( vertex_buffer_address_info );


	// Copying to the staging buffer on CPU
	auto staging_buffer = ManagedBuffer::Create( *mVMA->allocator.get(), vertex_buffer_size + index_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY );

	void* data = staging_buffer->allocation_info.pMappedData;
	memcpy( data, vertices.data(), vertex_buffer_size );
	memcpy( static_cast<char*>( data ) + vertex_buffer_size, indices.data(), index_buffer_size );

	// Copying to the GPU buffer
	ImmediateSubmit(
		[&]( vk::CommandBuffer& cmd )
		{
			vk::BufferCopy vertex_copy{};
			vertex_copy.size = vertex_buffer_size;
			vertex_copy.srcOffset = 0;
			vertex_copy.dstOffset = 0;

			cmd.copyBuffer( staging_buffer->buffer, new_surface.vertex_buffer->buffer, vertex_copy );

			vk::BufferCopy index_copy{};
			index_copy.size = index_buffer_size;
			index_copy.srcOffset = vertex_buffer_size;
			index_copy.dstOffset = 0;

			cmd.copyBuffer( staging_buffer->buffer, new_surface.index_buffer->buffer, index_copy );
		}
	);

	return std::make_shared<GPUMeshBuffers>( std::move( new_surface ) );
}

void
Renderer::SetCamera( std::shared_ptr<AbstractCamera> camera )
{
	mCamera = std::move( camera );
}

Renderer::Frame&
Renderer::GetCurrentFrame()
{
	return mFrames[ mFrameNumber % MAX_FRAMES_IN_FLIGHT ];
}

void
Renderer::Submit()
{
	auto& frame = GetCurrentFrame();

	auto submit_info = create_submit_info( 
		frame.command_context->GetSubmitInfo(), 
		frame.command_context->GetSwapchainSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput ),
		frame.command_context->GetPresentSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
	);

	mContext->graphics_queue.submit2( submit_info, frame.command_context->GetFence() );
}

void
Renderer::Present( uint32_t image_index )
{
	auto& frame = GetCurrentFrame();

	vk::PresentInfoKHR present_info{};
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &frame.command_context->GetPresentSemaphore();
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &mSwapChain->GetSwapChain();
	present_info.pImageIndices      = &image_index;

	std::ignore = mContext->present_queue.presentKHR( present_info );
}

void
Renderer::InitFrameResources()
{
	LOG( "Initializing frame resources ..." );
	{
		DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding( 0, vk::DescriptorType::eUniformBuffer );
		mRenderableFixedDescriptorSetLayout = layout_builder.Build( *mContext->logical_device, vk::ShaderStageFlagBits::eVertex );
	}

	for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		std::vector<DynamicDescriptorAllocator::PoolSizeRatio> frame_pool_ratio =
		{
			{ vk::DescriptorType::eStorageImage, 3 },
			{ vk::DescriptorType::eUniformBuffer, 3 },
			{ vk::DescriptorType::eStorageBuffer, 3 },
			{ vk::DescriptorType::eCombinedImageSampler, 4 }
		};
		Frame new_frame;
		new_frame.command_context = std::make_unique<VulkanCommandContext>( *mContext );
		new_frame.descriptor_allocator = std::make_unique<DynamicDescriptorAllocator>( *mContext->logical_device, DEFAULT_DESCRIPTOR_SET_COUNT, std::move( frame_pool_ratio ) );
		new_frame.renderable_descriptor_set = new_frame.descriptor_allocator->Allocate( *mRenderableFixedDescriptorSetLayout );
		new_frame.renderable_uniform_buffer = ManagedBuffer::Create( *mVMA->allocator.get(), sizeof( Renderable::UniformData ), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU );

		DescriptorWriter writer;
		writer.WriteBuffer( 0, vk::DescriptorType::eUniformBuffer, new_frame.renderable_uniform_buffer->buffer, 0, sizeof( Renderable::UniformData ) );
		writer.Update( *mContext->logical_device, new_frame.renderable_descriptor_set.get() );

		mFrames.push_back( std::move( new_frame ) );
	}
}

void
Renderer::InitDescriptors()
{
	LOG( "Initializing descriptor sets ..." );
	std::vector<DynamicDescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ vk::DescriptorType::eStorageImage, 1 }
	};
	mGlobalDescriptorAllocator = std::make_unique<DynamicDescriptorAllocator>( *mContext->logical_device, 10, sizes );

	{
		DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding( 0, vk::DescriptorType::eStorageImage );
		mRenderImageDescriptorSetLayout = layout_builder.Build( *mContext->logical_device, vk::ShaderStageFlagBits::eCompute );
	}

	mRenderImageDescriptorSet = mGlobalDescriptorAllocator->Allocate( *mRenderImageDescriptorSetLayout );

	DescriptorWriter writer;
	writer.WriteImage( 0, vk::DescriptorType::eStorageImage, mRenderImage->GetImageView(), vk::ImageLayout::eGeneral, nullptr );
	writer.Update( *mContext->logical_device, mRenderImageDescriptorSet.get() );
}

void
Renderer::InitPipelines()
{
	InitMeshPipeline();
}


bool
Renderer::InitMeshPipeline()
{
	auto vertex_shader = create_shader_module_from_file( *mContext->logical_device, "./src/shaders/compiled/colored_triangle_mesh.vert.spv" );
	if( !vertex_shader.has_value() )
	{
		LOG_ERROR( "Failed to create vertex shader module." );
		return false;
	}

	auto fragment_shader = create_shader_module_from_file( *mContext->logical_device, "./src/shaders/compiled/colored_triangle.frag.spv" );
	if( !fragment_shader.has_value() )
	{
		LOG_ERROR( "Failed to create fragment shader module." );
		return false;
	}

	vk::PipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &mRenderableFixedDescriptorSetLayout.get();
	mMeshPipelineLayout = mContext->logical_device->createPipelineLayoutUnique( pipeline_layout_info );

	VulkanPipelineBuilder pipeline_builder;
	mMeshPipeline = pipeline_builder
		.SetPipelineLayout( mMeshPipelineLayout.get() )
		.SetShaders( vertex_shader.value().get(), fragment_shader.value().get() )
		.SetInputTopology( vk::PrimitiveTopology::eTriangleList )
		.SetPolygonMode( vk::PolygonMode::eFill )
		.SetCullMode( vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise )
		.SetMultisampling()
		.SetBlendMode( vk::Bool32( VK_TRUE ), vk::BlendFactor::eOne, vk::BlendFactor::eDstAlpha, vk::BlendOp::eAdd )
		.SetColorAttachmentFormat( mRenderImage->GetFormat() )
		.SetDepthTest( vk::Bool32( VK_TRUE ), vk::Bool32( VK_TRUE ), vk::CompareOp::eGreaterOrEqual )
		.SetDepthFormat( mDepthImage->GetFormat() )
		.Build( *mContext->logical_device );

	return true;
}

void
Renderer::InitImGUI()
{
	mImGUILifetime = std::make_unique<ImGUILifetime>( *mContext );
	mImGUILifetime->Init( mWindow, MAX_FRAMES_IN_FLIGHT, static_cast<uint32_t>( mSwapChain->GetImages().size() ), mSwapChain->GetImageFormat() );

	ImmediateSubmit( []( vk::CommandBuffer& )
	{
		if( !ImGui_ImplVulkan_CreateFontsTexture() )
		{
			LOG_ERROR( "Failed to create IMGUI fonts texture." );
		}
	} );
}

void
Renderer::ImmediateSubmit( std::function<void( vk::CommandBuffer& )> work )
{
	auto& cmd = mImmidiateCommandContext->GetPrimaryBuffer();
	mImmidiateCommandContext->Reset();
	mImmidiateCommandContext->Begin();

	work( cmd );

	mImmidiateCommandContext->End();

	auto submit_info = create_submit_info(
		mImmidiateCommandContext->GetSubmitInfo(), std::nullopt, std::nullopt
	);

	mContext->graphics_queue.submit2( submit_info, mImmidiateCommandContext->GetFence() );
	mImmidiateCommandContext->WaitForCompletion();
}

void
Renderer::PrepareImGUI()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	// @TODO: Actual setup of the GUI should be passed in as a callback
	ImGui::ShowDemoWindow();
	ImGui::Render();
}

void
Renderer::DrawRenderables( vk::CommandBuffer& cmd )
{
	auto color_attachment = create_attachment_info( mRenderImage->GetImageView(), std::nullopt, vk::ImageLayout::eGeneral );
	vk::ClearValue depth_clear_value;
	depth_clear_value.depthStencil.depth = 0.f;
	auto depth_attachment = create_attachment_info( mDepthImage->GetImageView(), std::move( depth_clear_value ), vk::ImageLayout::eDepthAttachmentOptimal );

	auto render_extent = mRenderImage->GetExtent2D();
	vk::RenderingInfo render_info{};
	render_info.colorAttachmentCount = 1;
	render_info.pColorAttachments = &color_attachment;
	render_info.renderArea = vk::Rect2D{ vk::Offset2D{ 0, 0 }, std::move( render_extent ) };
	render_info.layerCount = 1;
	render_info.pDepthAttachment = &depth_attachment;

	cmd.beginRendering( render_info );
	cmd.bindPipeline( vk::PipelineBindPoint::eGraphics, mMeshPipeline.get() );

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

	auto projection_view_matrix = mCamera ? mCamera->GetViewProjectionMatrix() : glm::mat4( 1.0f );

	for( auto& renderable : mRenderQueue )
	{
		const auto* mesh_buffers = renderable->GetMeshBuffer();
		auto& frame = GetCurrentFrame();
		auto& descriptor_set = frame.renderable_descriptor_set;

		void* data;
		vmaMapMemory( *mVMA->allocator, frame.renderable_uniform_buffer->allocation, &data );
		auto uniform_data = renderable->GetFixedUniformData();
		uniform_data.project_view_matrix = projection_view_matrix;
		memcpy( data, &uniform_data, sizeof( Renderable::UniformData ) );
		vmaUnmapMemory( *mVMA->allocator, frame.renderable_uniform_buffer->allocation );

		cmd.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, mMeshPipelineLayout.get(), 0, 1, &descriptor_set.get(), 0, nullptr );

		cmd.bindIndexBuffer( mesh_buffers->index_buffer->buffer, 0, vk::IndexType::eUint32 );
		cmd.drawIndexed( 6, 1, 0, 0, 0 );
	}

	cmd.endRendering();
}
