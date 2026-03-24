#ifndef _EAGE_IMGUI_RENDER_PASS_H_
#define _EAGE_IMGUI_RENDER_PASS_H_

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <graphics/AbstractRenderPass.h>
#include <ecs/components/Hud.h>

union SDL_Event;
struct SDL_Window;
struct ImFont;

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

		/// Add a font at the given pixel size. Must be called before InitFontTexture.
		/// Pass nullptr for path to use the embedded default font.
		void LoadFont( const char* path, float size_px, eage::ecs::HudFontSize slot );

		/// Upload the ImGui font texture. Caller provides an immediate-submit function.
		void InitFontTexture( std::function<void( std::function<void( vk::CommandBuffer& )> )> immediate_submit );

		/// Retrieve a previously loaded font by HudFontSize slot.
		ImFont* GetFont( eage::ecs::HudFontSize slot ) const;

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

		std::array<ImFont*, static_cast<size_t>( eage::ecs::HudFontSize::COUNT )> mFonts{};
	};
}

#endif // _EAGE_IMGUI_RENDER_PASS_H_
