#ifndef _EAGE_COMMAND_BUFFER_H_
#define _EAGE_COMMAND_BUFFER_H_

namespace eage::graphics
{
	/// Opaque non-owning handle to a GPU command buffer.
	/// Only internal graphics code should cast GetNativeHandle() back to vk::CommandBuffer.
	class CommandBuffer
	{
	public:
		explicit CommandBuffer( void* native_handle ) noexcept
			: mNativeHandle( native_handle )
		{
		}

		[[nodiscard]] void* GetNativeHandle() const noexcept { return mNativeHandle; }

	private:
		void* mNativeHandle;
	};
}

#endif // _EAGE_COMMAND_BUFFER_H_
