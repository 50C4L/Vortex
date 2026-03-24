#ifndef _VULKAN_COMMAND_CONTEXT_H
#define _VULKAN_COMMAND_CONTEXT_H

#include <vulkan/vulkan_raii.hpp>

namespace eage::graphics
{
	class VulkanContext;

	///
	/// VulkanCommandContext class
	///
	class VulkanCommandContext
	{
	public:
		///
		/// Constructor
		///
		VulkanCommandContext( VulkanContext& context );

		///
		/// Destructor
		///
		virtual ~VulkanCommandContext();

		///
		/// Begin recording commands
		///
		void Begin();

		///
		/// End recording commands
		///
		vk::CommandBuffer& End();

		///
		/// Wait for the execution on the primary buffer to be completed.
		/// This function will block the caller thread until the primary buffer is available.
		/// 
		void WaitForCompletion();

		///
		/// Reset the command context
		///
		void Reset();

		///
		/// Get the primary command buffer
		///
		vk::CommandBuffer& GetPrimaryBuffer();

		///
		/// Get the primary command buffer
		///
		vk::CommandBufferSubmitInfo GetSubmitInfo() const;

		vk::Fence GetFence();
		///
		/// Get the queue family indices
		///
		/// @param stage_mask
		///  The stage mask to use
		///
		vk::SemaphoreSubmitInfo GetSwapchainSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask ) const;
		vk::SemaphoreSubmitInfo GetPresentSemaphoreSubmitInfo( vk::PipelineStageFlagBits2 stage_mask ) const;

		vk::Semaphore GetSwapchainSemaphore();
		vk::Semaphore GetPresentSemaphore();

	private:
		vk::raii::CommandPool   mCmdPool{ nullptr };		//< The guy that owns everything
		vk::CommandBuffer       mPrimaryBuffer;				//< Raw handle, lifetime managed by pool
		vk::raii::Fence         mFence{ nullptr };			//< Useful for synchronization
		vk::raii::Semaphore		mSwapchainSemaphore{ nullptr };
		vk::raii::Semaphore		mPresentSemaphore{ nullptr };

		VulkanContext& 			mContext;		//< The vulkan context
	};
} // namespace eage::graphics

#endif // _VULKAN_COMMAND_CONTEXT_H