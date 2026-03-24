#ifndef _EAGE_IMGUI_RENDER_PASS_H_
#define _EAGE_IMGUI_RENDER_PASS_H_

#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <graphics/AbstractRenderPass.h>

union SDL_Event;
struct SDL_Window;

namespace eage::graphics
{
	class ImGUILifetime;
	class VulkanContext;

	class ImGuiRenderPass final : public AbstractRenderPass
	{
	public:
		ImGuiRenderPass(
			VulkanContext& context,
			SDL_Window& window,
			vk::Format swapchain_format,
			uint32_t min_image_count,
			uint32_t max_image_count,
			ManagedImage& scene_color_target );

		~ImGuiRenderPass() override;

		/// Upload the ImGui font texture. Caller provides an immediate-submit function.
		void InitFontTexture( std::function<void( std::function<void( vk::CommandBuffer& )> )> immediate_submit );

		const RenderPassDesc& GetDesc() const override;

		void Prepare( size_t frame_index ) override;

		void Execute( vk::CommandBuffer& cmd, const ExecutionContext& ctx ) override;

		void AddOverlayCallback( std::function<void()> callback );

		void ProcessEvent( const SDL_Event& event );

	private:
		std::unique_ptr<ImGUILifetime> mLifetime;
		ManagedImage& mSceneColorTarget;
		RenderPassDesc mDesc;

		vk::UniqueSampler mSceneSampler;
		VkDescriptorSet mSceneDescriptorSet = VK_NULL_HANDLE;

		std::vector<std::function<void()>> mOverlayCallbacks;
	};
}

#endif // _EAGE_IMGUI_RENDER_PASS_H_
