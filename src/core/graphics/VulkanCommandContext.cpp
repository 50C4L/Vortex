#include "VulkanCommandContext.h"

#include "VulkanContext.h"

using namespace eage::graphics;

VulkanCommandContext::VulkanCommandContext( VulkanContext& context )
	: mContext( context )
{
	vk::CommandPoolCreateInfo command_pool_info{};
	command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	command_pool_info.queueFamilyIndex = mContext.queue_indices.graphics_family.value();

	mCmdPool = mContext.logical_device.createCommandPool( command_pool_info );

	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.commandPool = *mCmdPool;
	alloc_info.level = vk::CommandBufferLevel::ePrimary;
	alloc_info.commandBufferCount = 1;

	// Allocate via raw device - pool manages the buffer's lifetime
	vk::Device raw_device = *mContext.logical_device;
	mPrimaryBuffer = raw_device.allocateCommandBuffers( alloc_info )[0];

	// Create primary fence
	vk::FenceCreateInfo fence_info{};
	fence_info.flags = vk::FenceCreateFlagBits::eSignaled;
	mFence = mContext.logical_device.createFence( fence_info );

	vk::SemaphoreCreateInfo semaphore_info{};
	mSwapchainSemaphore = mContext.logical_device.createSemaphore( semaphore_info );
	mPresentSemaphore   = mContext.logical_device.createSemaphore( semaphore_info );
}

VulkanCommandContext::~VulkanCommandContext()
{
}

void
VulkanCommandContext::Begin()
{
	vk::CommandBufferBeginInfo begin_info{};
	mPrimaryBuffer.begin( begin_info );
}

vk::CommandBuffer&
VulkanCommandContext::End()
{
	mPrimaryBuffer.end();
	return mPrimaryBuffer;
}

void
VulkanCommandContext::WaitForCompletion()
{
	vk::Device raw_device = *mContext.logical_device;
	std::ignore = raw_device.waitForFences( *mFence, VK_TRUE, UINT64_MAX );
	raw_device.resetFences( *mFence );
}

void
VulkanCommandContext::Reset()
{
	vk::Device raw_device = *mContext.logical_device;
	raw_device.resetFences( *mFence );
	raw_device.resetCommandPool( *mCmdPool, vk::CommandPoolResetFlagBits::eReleaseResources );
}

vk::CommandBuffer&
VulkanCommandContext::GetPrimaryBuffer()
{
	return mPrimaryBuffer;
}

vk::CommandBufferSubmitInfo
VulkanCommandContext::GetSubmitInfo() const
{
	vk::CommandBufferSubmitInfo submit_info{};
	submit_info.commandBuffer = mPrimaryBuffer;
	submit_info.deviceMask = 0;

	return submit_info;
}

vk::Fence
VulkanCommandContext::GetFence()
{
	return *mFence;
}

vk::SemaphoreSubmitInfo
VulkanCommandContext::GetSwapchainSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask ) const
{
	vk::SemaphoreSubmitInfo semaphore_submit_info{};

	semaphore_submit_info.semaphore = *mSwapchainSemaphore;
	semaphore_submit_info.stageMask = stage_mask;
	semaphore_submit_info.deviceIndex = 0;
	semaphore_submit_info.value = 1;

	return semaphore_submit_info;
}

vk::SemaphoreSubmitInfo
VulkanCommandContext::GetPresentSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask ) const
{
	vk::SemaphoreSubmitInfo semaphore_submit_info{};

	semaphore_submit_info.semaphore = *mPresentSemaphore;
	semaphore_submit_info.stageMask = stage_mask;
	semaphore_submit_info.deviceIndex = 0;
	semaphore_submit_info.value = 1;

	return semaphore_submit_info;
}

vk::Semaphore
VulkanCommandContext::GetSwapchainSemaphore()
{
	return *mSwapchainSemaphore;
}

vk::Semaphore
VulkanCommandContext::GetPresentSemaphore()
{
	return *mPresentSemaphore;
}