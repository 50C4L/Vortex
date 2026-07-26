#ifndef _EAGE_IMGUI_RENDER_PASS_H_
#define _EAGE_IMGUI_RENDER_PASS_H_

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <graphics/AbstractRenderPass.h>
#include <ecs/components/Hud.h>

union SDL_Event;
struct SDL_Window;
struct ImFont;

namespace eage::graphics
{
	class VulkanContext;
}

namespace eage::imgui
{
	class ImGUILifetime;

	class ImGuiRenderPass final : public eage::graphics::AbstractRenderPass
	{
	public:
		ImGuiRenderPass(
			eage::graphics::VulkanContext& context,
			SDL_Window& window,
			vk::Format swapchain_format,
			uint32_t min_image_count,
			uint32_t max_image_count );

		~ImGuiRenderPass() override;

		/// Add a font at the given pixel size. Must be called before InitFontTexture.
		/// Pass nullptr for path to use the embedded default font.
		void LoadFont( const char* path, float size_px, eage::ecs::HudFontSize slot );

		/// Upload the ImGui font texture. Caller provides an immediate-submit function.
		void InitFontTexture( std::function<void( std::function<void( vk::CommandBuffer& )> )> immediate_submit );

		/// Retrieve a previously loaded font by HudFontSize slot.
		ImFont* GetFont( eage::ecs::HudFontSize slot ) const;

		const eage::graphics::RenderPassDesc& GetDesc() const override;

		void Prepare( size_t frame_index ) override;

		void Execute( eage::graphics::CommandBuffer& cmd, const eage::graphics::FrameContext& ctx ) override;

		void AddOverlayCallback( std::function<void()> callback );

		void ProcessEvent( const SDL_Event& event );

		/// Retarget the scene color image sampled by tool UI (e.g. AnimTool preview).
		/// Pass nullptr to clear. Not used by the game shell present path.
		void SetSceneInput( eage::graphics::ManagedImage* image );

		/// When set, Execute clears the swapchain with this color (loadOp eClear).
		/// When nullopt, Execute loads existing contents (loadOp eLoad) for overlay compositing.
		void SetClearColor( std::optional<glm::vec4> color );

		void* GetSceneTextureId() const;

	private:
		std::unique_ptr<ImGUILifetime> mLifetime;
		eage::graphics::ManagedImage* mSceneColorTarget = nullptr;
		eage::graphics::RenderPassDesc mDesc;

		vk::UniqueSampler mSceneSampler;
		VkDescriptorSet mSceneDescriptorSet = VK_NULL_HANDLE;

		std::vector<std::function<void()>> mOverlayCallbacks;

		std::array<ImFont*, static_cast<size_t>( eage::ecs::HudFontSize::COUNT )> mFonts{};
	};
}

#endif // _EAGE_IMGUI_RENDER_PASS_H_
