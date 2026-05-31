#ifndef _EAGE_IMGUI_HUD_RENDERER_H_
#define _EAGE_IMGUI_HUD_RENDERER_H_

#include <ecs/systems/AbstractHudRenderer.h>

namespace eage::graphics
{
	class ImGuiRenderPass;

	///
	/// Adaptor that implements AbstractHudRenderer on top of ImGuiRenderPass.
	/// Keeps the render-pass class free of HUD rendering responsibilities.
	///
	class ImGuiHudRenderer : public eage::ecs::AbstractHudRenderer
	{
	public:
		explicit ImGuiHudRenderer( ImGuiRenderPass& imgui_pass );

		void RegisterRenderCallback( std::function<void()> callback ) override;
		glm::vec2 GetViewportSize() const override;
		glm::vec2 MeasureText( const std::string& text, eage::ecs::HudFontSize size ) const override;
		void DrawText( glm::vec2 screen_pos, const std::string& text, glm::vec4 color, eage::ecs::HudFontSize size ) override;

	private:
		ImGuiRenderPass& mImGuiPass;
	};
}

#endif // _EAGE_IMGUI_HUD_RENDERER_H_
