#include "VulkanCommandContext.h"

#include "VulkanContext.h"

using namespace graphics;

VulkanCommandContext::VulkanCommandContext( VulkanContext& context )
	: mContext( context )
{
	vk::CommandPoolCreateInfo command_pool_info{};
	command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	command_pool_info.queueFamilyIndex = mContext.queue_indices.graphics_family.value();

	try
	{
		mCmdPool = mContext.logical_device->createCommandPoolUnique( command_pool_info );
	}
	catch ( vk::SystemError /*err*/ )
	{
		throw std::runtime_error( "Failed to create command pool!" );
	}

	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.commandPool = mCmdPool.get();
	alloc_info.level = vk::CommandBufferLevel::ePrimary;
	alloc_info.commandBufferCount = 1;

	try
	{
		mPrimaryBuffer = std::move( mContext.logical_device->allocateCommandBuffersUnique( alloc_info )[0] );
	}
	catch ( vk::SystemError /*error*/ )
	{
		throw std::runtime_error( "Failed to create main command buffers." );
	}

	// Create primary fence
	vk::FenceCreateInfo fence_info{};
	fence_info.flags = vk::FenceCreateFlagBits::eSignaled;
	try
	{
		mFence = mContext.logical_device->createFenceUnique( fence_info );
	}
	catch (vk::SystemError /*error*/)
	{
		throw std::runtime_error( "Failed to create synchronization fences!" );
	}

	vk::SemaphoreCreateInfo semaphore_info{};
	try
	{
		mSwapchainSemaphore = mContext.logical_device->createSemaphoreUnique( semaphore_info );
		mPresentSemaphore   = mContext.logical_device->createSemaphoreUnique( semaphore_info );
	}
	catch( vk::SystemError /*error*/ )
	{
		throw std::runtime_error( "Failed to create synchronization semaphores!" );
	}
}

VulkanCommandContext::~VulkanCommandContext()
{
}

void
VulkanCommandContext::Begin()
{
	vk::CommandBufferBeginInfo begin_info{};
	mPrimaryBuffer->begin( begin_info );
}

vk::CommandBuffer&
VulkanCommandContext::End()
{
	mPrimaryBuffer->end();
	return *mPrimaryBuffer;
}

void
VulkanCommandContext::WaitForCompletion()
{
	mContext.logical_device->waitForFences( *mFence, VK_TRUE, UINT64_MAX );
	mContext.logical_device->resetFences( *mFence );
}

void
VulkanCommandContext::Reset()
{
	mContext.logical_device->resetFences( *mFence );
	mContext.logical_device->resetCommandPool( *mCmdPool, vk::CommandPoolResetFlagBits::eReleaseResources );
}

vk::CommandBuffer&
VulkanCommandContext::GetPrimaryBuffer()
{
	return *mPrimaryBuffer;
}

vk::CommandBufferSubmitInfo
VulkanCommandContext::GetSubmitInfo() const
{
	vk::CommandBufferSubmitInfo submit_info{};
	submit_info.commandBuffer = *mPrimaryBuffer;
	submit_info.deviceMask = 0;

	return submit_info;
}

vk::Fence&
VulkanCommandContext::GetFence()
{
	return *mFence;
}

vk::SemaphoreSubmitInfo
VulkanCommandContext::GetSwapchainSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask ) const
{
	vk::SemaphoreSubmitInfo semaphore_submit_info{};

	semaphore_submit_info.semaphore = mSwapchainSemaphore.get();
	semaphore_submit_info.stageMask = stage_mask;
	semaphore_submit_info.deviceIndex = 0;
	semaphore_submit_info.value = 1;

	return semaphore_submit_info;
}

vk::SemaphoreSubmitInfo
VulkanCommandContext::GetPresentSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask ) const
{
	vk::SemaphoreSubmitInfo semaphore_submit_info{};

	semaphore_submit_info.semaphore = mPresentSemaphore.get();
	semaphore_submit_info.stageMask = stage_mask;
	semaphore_submit_info.deviceIndex = 0;
	semaphore_submit_info.value = 1;

	return semaphore_submit_info;
}

vk::Semaphore&
VulkanCommandContext::GetSwapchainSemaphore()
{
	return mSwapchainSemaphore.get();
}

vk::Semaphore&
VulkanCommandContext::GetPresentSemaphore()
{
	return mPresentSemaphore.get();
}