#ifndef _VULKAN_COMMAND_CONTEXT_H
#define _VULKAN_COMMAND_CONTEXT_H

#include <vulkan/vulkan.hpp>

namespace graphics
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

		vk::Fence& GetFence();

	private:
		vk::UniqueCommandPool   mCmdPool;		//< The guy that owns everything
		vk::UniqueCommandBuffer mPrimaryBuffer; //< The primary command queue
		vk::UniqueFence         mFence;			//< Useful for synchronization
		VulkanContext& 			mContext;		//< The vulkan context
	};
} // namespace graphics

#endif // _VULKAN_COMMAND_CONTEXT_H