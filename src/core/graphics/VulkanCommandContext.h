#ifndef _VULKAN_COMMAND_CONTEXT_H
#define _VULKAN_COMMAND_CONTEXT_H

namespace graphics
{
	///
	/// VulkanCommandContext class
	///
	class VulkanCommandContext
	{
	public:
		///
		/// Constructor
		///
		VulkanCommandContext();

		///
		/// Destructor
		///
		virtual ~VulkanCommandContext();

		///
		/// Initialize the command context
		///
		/// @return
		///  true if successful, false otherwise
		///
		bool Init();

		///
		/// Begin recording commands
		///
		void Begin();

		///
		/// End recording commands
		///
		void End();

		///
		/// Submit the command buffer
		///
		void Submit();
	};
} // namespace graphics

#endif // _VULKAN_COMMAND_CONTEXT_H