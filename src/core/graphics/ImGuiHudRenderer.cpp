#include "ImGuiHudRenderer.h"

#include <imgui/imgui.h>

#include <graphics/ImGuiRenderPass.h>

using namespace eage::graphics;
using namespace eage::ecs;

ImGuiHudRenderer::ImGuiHudRenderer( ImGuiRenderPass& imgui_pass )
	: mImGuiPass( imgui_pass )
{
}

void
ImGuiHudRenderer::RegisterRenderCallback( std::function<void()> callback )
{
	mImGuiPass.AddOverlayCallback( std::move( callback ) );
}

glm::vec2
ImGuiHudRenderer::GetViewportSize() const
{
	const ImGuiViewport* vp = ImGui::GetMainViewport();
	return glm::vec2{ vp->Size.x, vp->Size.y };
}

glm::vec2
ImGuiHudRenderer::MeasureText( const std::string& text, HudFontSize size ) const
{
	ImFont* font = mImGuiPass.GetFont( size );
	ImVec2 text_size = font->CalcTextSizeA( font->FontSize, FLT_MAX, 0.0f, text.c_str() );
	return glm::vec2{ text_size.x, text_size.y };
}

void
ImGuiHudRenderer::DrawText( glm::vec2 screen_pos, const std::string& text, glm::vec4 color, HudFontSize size )
{
	ImFont* font = mImGuiPass.GetFont( size );
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	ImU32 imgui_color = ImGui::ColorConvertFloat4ToU32( ImVec4( color.r, color.g, color.b, color.a ) );
	draw_list->AddText( font, font->FontSize, ImVec2{ screen_pos.x, screen_pos.y }, imgui_color, text.c_str() );
}
