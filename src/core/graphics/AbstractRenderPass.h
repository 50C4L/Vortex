#ifndef _EAGE_ABSTRACT_RENDER_PASS_H_
#define _EAGE_ABSTRACT_RENDER_PASS_H_

#include <vector>
#include <optional>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

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

	struct ExecutionContext
	{
		vk::ImageView  swapchain_image_view;
		vk::Extent2D   swapchain_extent;
		size_t         frame_index;
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
		virtual void Execute( vk::CommandBuffer& cmd, const ExecutionContext& ctx ) = 0;
	};
}

#endif // _EAGE_ABSTRACT_RENDER_PASS_H_
