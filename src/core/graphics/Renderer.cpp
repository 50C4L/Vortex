#include "Renderer.h"

#include <algorithm>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_system.h>
#include <iostream>

#include <utility/Logger.h>
#include <graphics/AbstractRenderPass.h>
#include <graphics/ImageUtilities.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VulkanContext.h>
#include <graphics/VulkanSwapChain.h>
#include <graphics/VulkanCommandContext.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VMAWrapper.h>

using namespace eage::graphics;
using namespace utility;

namespace
{
	const uint32_t DEFAULT_DESCRIPTOR_SET_COUNT = 1000u;

	struct SubmitInfoBundle
	{
		vk::CommandBufferSubmitInfo cmd_info{};
		vk::SemaphoreSubmitInfo wait_info{};
		vk::SemaphoreSubmitInfo signal_info{};
		vk::SubmitInfo2 submit_info{};

		SubmitInfoBundle(
			vk::CommandBufferSubmitInfo cmd_submit_info,
			std::optional<vk::SemaphoreSubmitInfo> semaphore_wait_info,
			std::optional<vk::SemaphoreSubmitInfo> semaphore_signal_info )
			: cmd_info( cmd_submit_info )
		{
			submit_info.commandBufferInfoCount = 1;
			submit_info.pCommandBufferInfos = &cmd_info;

			if( semaphore_wait_info.has_value() )
			{
				wait_info = semaphore_wait_info.value();
				submit_info.waitSemaphoreInfoCount = 1;
				submit_info.pWaitSemaphoreInfos = &wait_info;
			}

			if( semaphore_signal_info.has_value() )
			{
				signal_info = semaphore_signal_info.value();
				submit_info.signalSemaphoreInfoCount = 1;
				submit_info.pSignalSemaphoreInfos = &signal_info;
			}
		}
	};
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

	mSwapchainLayouts.assign( mSwapChain->GetImages().size(), vk::ImageLayout::eUndefined );

	LOG( "Creating render image ..." );

	mRenderImage = ManagedImage::Create(
		*mContext->logical_device,
		*mVMA->allocator.get(),
		vk::Extent3D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 },
		vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::ImageAspectFlagBits::eColor
	);

	LOG( "Creating depth image ..." );

	mDepthImage = ManagedImage::Create(
		*mContext->logical_device,
		*mVMA->allocator.get(),
		vk::Extent3D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 },
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::ImageAspectFlagBits::eDepth
	);

	// Init descriptor set layout
	InitDescriptors();
	InitBindless();

	// Initialize GPU timing
	InitGPUTiming();

	return true;
}

void
Renderer::Render()
{
	// CPU-side preparation for all passes
	size_t current_frame = GetCurrentFrameIndex();
	for( auto* pass : mPasses )
	{
		pass->Prepare( current_frame );
	}

	auto& frame = GetCurrentFrame();
	auto& cmd = frame.command_context->GetPrimaryBuffer();
	uint32_t next_image_index = mSwapChain->GetNextImage( frame.command_context->GetSwapchainSemaphore() );

	frame.command_context->WaitForCompletion();
	frame.command_context->Reset();
	frame.command_context->Begin();

	// Acquired swapchain images have undefined contents; PresentPass covers the full extent.
	mSwapchainLayouts[ next_image_index ] = vk::ImageLayout::eUndefined;

	// GPU timing: Record start timestamp
	if( mTimestampQuerySupported )
	{
		uint32_t query_index = static_cast<uint32_t>( current_frame ) * 2u;
		cmd.resetQueryPool( mTimestampQueryPool.get(), query_index, 2 );
		cmd.writeTimestamp( vk::PipelineStageFlagBits::eTopOfPipe, mTimestampQueryPool.get(), query_index );
	}

	// Execute each registered render pass
	for( auto* pass : mPasses )
	{
		const auto& desc = pass->GetDesc();

		// Transition input images to the layout required by their access mode
		for( const auto& input : desc.input_images )
		{
			vk::ImageLayout layout = ( input.access == ImageAccess::TransferSrc )
				? vk::ImageLayout::eTransferSrcOptimal
				: vk::ImageLayout::eShaderReadOnlyOptimal;
			TransitionImage( cmd, *input.image, layout );
		}

		// Transition color target (off-screen) or swapchain
		if( desc.color_target )
		{
			TransitionImage( cmd, *desc.color_target, vk::ImageLayout::eColorAttachmentOptimal );
		}
		else if( desc.swapchain_access == SwapchainAccess::ColorAttachment )
		{
			TransitionSwapchainImage( cmd, next_image_index, vk::ImageLayout::eColorAttachmentOptimal );
		}
		else if( desc.swapchain_access == SwapchainAccess::TransferDst )
		{
			TransitionSwapchainImage( cmd, next_image_index, vk::ImageLayout::eTransferDstOptimal );
		}

		// Transition depth target
		if( desc.depth_target )
		{
			TransitionImage( cmd, *desc.depth_target, vk::ImageLayout::eDepthAttachmentOptimal );
		}

		CommandBuffer cmd_buffer( static_cast<void*>( cmd  ) );

		FrameContext frame_ctx{};
		frame_ctx.swapchain_image_handle = static_cast<void*>( static_cast<VkImage>( mSwapChain->GetImages()[ next_image_index ] ) );
		frame_ctx.swapchain_image_view_handle = static_cast<void*>( static_cast<VkImageView>( *mSwapChain->GetImageViews()[ next_image_index ] ) );
		frame_ctx.swapchain_width = mSwapChain->GetExtent().width;
		frame_ctx.swapchain_height = mSwapChain->GetExtent().height;
		frame_ctx.frame_index = current_frame;

		pass->Execute( cmd_buffer, frame_ctx );
	}

	// Transition swapchain to present using the tracked layout (may be TransferDst or ColorAttachment)
	TransitionSwapchainImage( cmd, next_image_index, vk::ImageLayout::ePresentSrcKHR );

	// GPU timing: Record end timestamp
	if( mTimestampQuerySupported )
	{
		uint32_t query_index = static_cast<uint32_t>( current_frame ) * 2u + 1u;
		cmd.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, mTimestampQueryPool.get(), query_index );
	}
	
	frame.command_context->End();

	Submit();

	// Update GPU timing after submission
	UpdateGPUTiming();

	Present( next_image_index );

	mFrameNumber++;
}

void
Renderer::WaitForIdle()
{
	mContext->logical_device.waitIdle();
}

std::unique_ptr<GPUMeshBuffers>
Renderer::UploadMesh( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices )
{
	const size_t vertex_buffer_size = sizeof( Vertex ) * vertices.size();
	const size_t index_buffer_size = sizeof( uint32_t ) * indices.size();

	GPUMeshBuffers new_surface{
		ManagedBuffer::Create( *mVMA->allocator.get(), index_buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY ),
		ManagedBuffer::Create( *mVMA->allocator.get(), vertex_buffer_size, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY )
	};

	vk::BufferDeviceAddressInfo vertex_buffer_address_info{};
	vertex_buffer_address_info.buffer = new_surface.vertex_buffer->buffer;
	new_surface.vertex_buffer_address = mContext->logical_device.getBufferAddress( vertex_buffer_address_info );

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

	return std::make_unique<GPUMeshBuffers>( std::move( new_surface ) );
}

ManagedImage::Ptr
Renderer::UploadImage(
	void* data,
	size_t image_size,
	uint32_t width, uint32_t height,
	vk::Format format,
	vk::ImageUsageFlags usage,
	vk::ImageAspectFlags aspect_flags,
	uint32_t mip_levels )
{
	auto upload_buffer = ManagedBuffer::Create( *mVMA->allocator.get(), image_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU );

	memcpy( upload_buffer->allocation_info.pMappedData, data, image_size );

	auto image = ManagedImage::Create( 
		*mContext->logical_device,
		*mVMA->allocator.get(),
		vk::Extent3D{ width, height, 1 },
		format,
		usage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		aspect_flags,
		mip_levels
	);

	// Copying to the GPU buffer
	ImmediateSubmit(
		[&]( vk::CommandBuffer& cmd )
		{
			transition_image( cmd, image->image, image->current_layout, vk::ImageLayout::eTransferDstOptimal );
			image->current_layout = vk::ImageLayout::eTransferDstOptimal;

			vk::BufferImageCopy copy{};
			copy.bufferOffset = 0;
			copy.bufferRowLength = 0;
			copy.bufferImageHeight = 0;

			copy.imageSubresource.aspectMask = aspect_flags;
			copy.imageSubresource.mipLevel = 0;
			copy.imageSubresource.baseArrayLayer = 0;
			copy.imageSubresource.layerCount = 1;
			copy.imageOffset = vk::Offset3D{ 0, 0, 0 };
			copy.imageExtent = vk::Extent3D{ width, height, 1 };

			cmd.copyBufferToImage( upload_buffer->buffer, image->image, vk::ImageLayout::eTransferDstOptimal, 1, &copy );

			transition_image( cmd, image->image, image->current_layout, vk::ImageLayout::eShaderReadOnlyOptimal );
			image->current_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		}
	);

	return image;
}

vk::UniqueSampler
Renderer::CreateSampler( vk::Filter min_filter, vk::Filter mag_filter )
{
	vk::SamplerCreateInfo sampler_info{};
	sampler_info.magFilter = mag_filter;
	sampler_info.minFilter = min_filter;
	// sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
	// sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	// sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
	// sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
	// sampler_info.mipLodBias = 0.0f;
	// sampler_info.anisotropyEnable = VK_FALSE;
	// sampler_info.maxAnisotropy = 1.0f;
	// sampler_info.compareEnable = VK_FALSE;
	// sampler_info.compareOp = vk::CompareOp::eAlways;
	// sampler_info.minLod = 0.0f;
	// sampler_info.maxLod = 0.0f;
	// sampler_info.borderColor = vk::BorderColor::eFloatOpaqueBlack;
	// sampler_info.unnormalizedCoordinates = VK_FALSE;

	vk::Device raw_device = *mContext->logical_device;
	return raw_device.createSamplerUnique( sampler_info );
}

vk::Device
Renderer::GetDevice()
{
	return *mContext->logical_device;
}

std::vector<Renderer::Frame>&
Renderer::GetFrames()
{
	return mFrames;
}

VMAWrapper&
Renderer::GetMemoryAllocator()
{
	return *mVMA;
}

Renderer::Frame&
Renderer::GetCurrentFrame()
{
	return mFrames[ GetCurrentFrameIndex() ];
}

vk::Format
Renderer::GetDepthFormat()
{
	return mDepthImage->format;
}

vk::Format
Renderer::GetColorFormat()
{
	return mRenderImage->format;
}

size_t
Renderer::GetCurrentFrameIndex() const
{
	return mFrameNumber % MAX_FRAMES_IN_FLIGHT;
}

Renderer::BuiltInDescriptorSetLayouts&
Renderer::GetBuiltInDescriptorSetLayouts()
{
	return mBuiltInDescriptorSetLayouts;
}

void
Renderer::Submit()
{
	auto& frame = GetCurrentFrame();

	auto submit_bundle = SubmitInfoBundle(
		frame.command_context->GetSubmitInfo(),
		frame.command_context->GetSwapchainSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput ),
		frame.command_context->GetPresentSemaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
	);

	mContext->graphics_queue.submit2( submit_bundle.submit_info, frame.command_context->GetFence() );
}

void
Renderer::Present( uint32_t image_index )
{
	auto& frame = GetCurrentFrame();

	vk::Semaphore present_semaphore = frame.command_context->GetPresentSemaphore();
	vk::PresentInfoKHR present_info{};
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores	= &present_semaphore;
	present_info.swapchainCount	 = 1;
	present_info.pSwapchains		= &mSwapChain->GetSwapChain();
	present_info.pImageIndices	  = &image_index;

	vk::Queue raw_present_queue = *mContext->present_queue;
	std::ignore = raw_present_queue.presentKHR( present_info );
}

void
Renderer::InitFrameResources()
{
	LOG( "Initializing frame resources ..." );
	for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		std::vector<DynamicDescriptorAllocator::PoolSizeRatio> frame_pool_ratio =
		{
			{ vk::DescriptorType::eStorageImage, 3 },
			{ vk::DescriptorType::eUniformBuffer, 3 },
			{ vk::DescriptorType::eUniformBufferDynamic, 3 },
			{ vk::DescriptorType::eStorageBuffer, 3 },
			{ vk::DescriptorType::eCombinedImageSampler, 4 }
		};
		Frame new_frame;
		new_frame.command_context = std::make_unique<VulkanCommandContext>( *mContext );
		new_frame.descriptor_allocator = std::make_unique<DynamicDescriptorAllocator>( *mContext->logical_device, DEFAULT_DESCRIPTOR_SET_COUNT, std::move( frame_pool_ratio ) );

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
	mGlobalDescriptorAllocator = std::make_unique<DynamicDescriptorAllocator>( *mContext->logical_device, DEFAULT_DESCRIPTOR_SET_COUNT, sizes );

	// Scene global layout (view/projection matrices)
	{
		DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding( 0, vk::DescriptorType::eUniformBufferDynamic );
		mBuiltInDescriptorSetLayouts.global = 
			layout_builder.Build( *mContext->logical_device, vk::ShaderStageFlagBits::eVertex );
	}

	// Render component layout (model matrix, vertex buffer address)
	{
		DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding( 0, vk::DescriptorType::eUniformBufferDynamic );
		mBuiltInDescriptorSetLayouts.per_object = 
			layout_builder.Build( *mContext->logical_device, vk::ShaderStageFlagBits::eVertex );
	}
}

void
Renderer::InitBindless()
{
	LOG( "Initializing bindless texture array ..." );

	{
		DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding(
			0,
			vk::DescriptorType::eCombinedImageSampler,
			MAX_BINDLESS_TEXTURES,
			vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound );
		mBuiltInDescriptorSetLayouts.bindless =
			layout_builder.Build( *mContext->logical_device, vk::ShaderStageFlagBits::eFragment );
	}

	vk::DescriptorPoolSize pool_size( vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_TEXTURES );
	vk::DescriptorPoolCreateInfo pool_info{};
	// UniqueDescriptorSet destructor frees the set, so the pool must allow it.
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
		| vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	mBindlessPool = ( *mContext->logical_device ).createDescriptorPoolUnique( pool_info );

	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.descriptorPool = mBindlessPool.get();
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &mBuiltInDescriptorSetLayouts.bindless.get();
	mBindlessSet = std::move( ( *mContext->logical_device ).allocateDescriptorSetsUnique( alloc_info )[0] );

	mDefaultSampler = CreateSampler( vk::Filter::eNearest, vk::Filter::eNearest );

	// Reserve index 0 for a 1x1 fallback texture (bright magenta — obvious missing/wrong texture)
	uint32_t fallback_pixels[1] = { 0xFFFF00FF };
	auto fallback_image = UploadImage(
		fallback_pixels,
		sizeof( fallback_pixels ),
		1, 1,
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageUsageFlagBits::eSampled,
		vk::ImageAspectFlagBits::eColor,
		1 );
	RegisterBindlessTexture( fallback_image->image_view.get(), mDefaultSampler.get() );
}

uint32_t
Renderer::RegisterBindlessTexture( vk::ImageView image_view, vk::Sampler sampler )
{
	if( mNextBindlessIndex >= MAX_BINDLESS_TEXTURES )
	{
		LOG_ERROR( "Bindless texture array is full" );
		return 0;
	}

	const uint32_t index = mNextBindlessIndex++;

	vk::DescriptorImageInfo image_info( sampler, image_view, vk::ImageLayout::eShaderReadOnlyOptimal );
	vk::WriteDescriptorSet write{};
	write.dstSet = mBindlessSet.get();
	write.dstBinding = 0;
	write.dstArrayElement = index;
	write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	write.descriptorCount = 1;
	write.pImageInfo = &image_info;
	GetDevice().updateDescriptorSets( 1, &write, 0, nullptr );

	return index;
}

vk::DescriptorSet
Renderer::GetBindlessDescriptorSet() const
{
	return mBindlessSet.get();
}

vk::Sampler
Renderer::GetDefaultSampler() const
{
	return mDefaultSampler.get();
}

void
Renderer::AddRenderPass( AbstractRenderPass* pass )
{
	mPasses.push_back( pass );
}

void
Renderer::RemoveRenderPass( AbstractRenderPass* pass )
{
	mPasses.erase( std::remove( mPasses.begin(), mPasses.end(), pass ), mPasses.end() );
}

void
Renderer::TransitionImage( vk::CommandBuffer& cmd, ManagedImage& image, vk::ImageLayout new_layout )
{
	// Always emit the barrier, even when layouts match -- transition_image is also
	// the execution barrier between passes.
	transition_image( cmd, image.image, image.current_layout, new_layout );
	image.current_layout = new_layout;
}

void
Renderer::TransitionSwapchainImage( vk::CommandBuffer& cmd, uint32_t index, vk::ImageLayout new_layout )
{
	transition_image( cmd, mSwapChain->GetImages()[ index ], mSwapchainLayouts[ index ], new_layout );
	mSwapchainLayouts[ index ] = new_layout;
}

VulkanContext&
Renderer::GetVulkanContext()
{
	return *mContext;
}

vk::Format
Renderer::GetSwapchainFormat()
{
	return mSwapChain->GetImageFormat();
}

uint32_t
Renderer::GetSwapchainImageCount()
{
	return static_cast<uint32_t>( mSwapChain->GetImages().size() );
}

ManagedImage*
Renderer::GetRenderImage()
{
	return mRenderImage.get();
}

ManagedImage*
Renderer::GetDepthImage()
{
	return mDepthImage.get();
}

void
Renderer::ImmediateSubmit( std::function<void( vk::CommandBuffer& )> work )
{
	auto& cmd = mImmidiateCommandContext->GetPrimaryBuffer();
	mImmidiateCommandContext->Reset();
	mImmidiateCommandContext->Begin();

	work( cmd );

	mImmidiateCommandContext->End();

	auto submit_bundle = SubmitInfoBundle(
		mImmidiateCommandContext->GetSubmitInfo(), std::nullopt, std::nullopt
	);

	mContext->graphics_queue.submit2( submit_bundle.submit_info, mImmidiateCommandContext->GetFence() );
	mImmidiateCommandContext->WaitForCompletion();
}

void
Renderer::InitGPUTiming()
{
	// Check if timestamp queries are supported
	auto queue_family_props = mContext->physical_device.getQueueFamilyProperties();
	mTimestampQuerySupported = queue_family_props[*mContext->queue_indices.graphics_family].timestampValidBits > 0;
	
	if( !mTimestampQuerySupported )
	{
		LOG_ERROR() << "Timestamp queries not supported on this device";
		return;
	}
	
	// Create query pool for timestamps
	vk::QueryPoolCreateInfo query_pool_info{};
	query_pool_info.queryType = vk::QueryType::eTimestamp;
	query_pool_info.queryCount = MAX_FRAMES_IN_FLIGHT * 2; // Start and end per frame
	
	mTimestampQueryPool = ( *mContext->logical_device ).createQueryPoolUnique( query_pool_info );
	
	// Reset the query pool
	auto& cmd = mImmidiateCommandContext->GetPrimaryBuffer();
	mImmidiateCommandContext->Reset();
	mImmidiateCommandContext->Begin();
	cmd.resetQueryPool( mTimestampQueryPool.get(), 0, MAX_FRAMES_IN_FLIGHT * 2 );
	mImmidiateCommandContext->End();
	
	auto submit_bundle = SubmitInfoBundle(
		mImmidiateCommandContext->GetSubmitInfo(), std::nullopt, std::nullopt
	);
	mContext->graphics_queue.submit2( submit_bundle.submit_info, mImmidiateCommandContext->GetFence() );
	mImmidiateCommandContext->WaitForCompletion();
	
	LOG() << "GPU timing initialized successfully";
}

void
Renderer::UpdateGPUTiming()
{
	if( !mTimestampQuerySupported )
	{
		mGPUFrameTime = 0.0f;
		return;
	}
	
	// Get timestamps from the previous frame (avoid reading current frame's incomplete results)
	uint32_t prev_frame_index = (GetCurrentFrameIndex() + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
	uint32_t start_query = prev_frame_index * 2;
	uint32_t end_query = prev_frame_index * 2 + 1;
	
	try
	{
		// Read timestamp results
		vk::Device raw_device = *mContext->logical_device;
		auto result = raw_device.getQueryPoolResults(
			mTimestampQueryPool.get(),
			start_query,
			2, // Read 2 queries (start and end)
			sizeof(uint64_t) * 2,
			&mTimestampResults[start_query],
			sizeof(uint64_t),
			vk::QueryResultFlagBits::e64
		);
		
		if( result == vk::Result::eSuccess )
		{
			uint64_t start_timestamp = mTimestampResults[start_query];
			uint64_t end_timestamp = mTimestampResults[end_query];
			
			if( end_timestamp > start_timestamp )
			{
				// Convert to milliseconds
				auto physical_device_props = mContext->physical_device.getProperties();
				float timestamp_period = physical_device_props.limits.timestampPeriod; // In nanoseconds
				
				uint64_t elapsed_ticks = end_timestamp - start_timestamp;
				mGPUFrameTime = (elapsed_ticks * timestamp_period) / 1000000.0f; // Convert to milliseconds
			}
		}
	}
	catch( const vk::SystemError& )
	{
		// Query results not ready yet - keep previous value
		// This is normal during the first few frames
	}
}
