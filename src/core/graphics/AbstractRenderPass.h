#ifndef _EAGE_ABSTRACT_RENDER_PASS_H_
#define _EAGE_ABSTRACT_RENDER_PASS_H_

#include <cstdint>
#include <vector>
#include <optional>

#include <glm/glm.hpp>

#include <graphics/CommandBuffer.h>

namespace eage::graphics
{
	struct ManagedImage;

	struct RenderPassDesc
	{
		ManagedImage*              color_target = nullptr;
		ManagedImage*              depth_target = nullptr;
		std::vector<ManagedImage*> input_images;
		std::optional<glm::vec4>   clear_color;
		std::optional<float>       clear_depth;
	};

	/// Per-frame execution context passed to AbstractRenderPass::Execute().
	/// All platform-specific handle types are stored as opaque void* to avoid
	/// leaking Vulkan types through this interface.
	struct FrameContext
	{
		void*    swapchain_image_view_handle; ///< VkImageView; cast in graphics internals only.
		uint32_t swapchain_width;
		uint32_t swapchain_height;
		size_t   frame_index;
	};

	class AbstractRenderPass
	{
	public:
		virtual ~AbstractRenderPass() = default;

		virtual const RenderPassDesc& GetDesc() const = 0;

		/// CPU-side work before command recording (e.g. ImGui::NewFrame).
		virtual void Prepare( size_t frame_index )
		{
		}

		/// Record GPU commands into cmd. Barriers already inserted by Renderer.
		virtual void Execute( CommandBuffer& cmd, const FrameContext& ctx ) = 0;
	};
}

#endif // _EAGE_ABSTRACT_RENDER_PASS_H_
