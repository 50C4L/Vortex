#ifndef _ANIMTOOL_UI_STYLE_H_
#define _ANIMTOOL_UI_STYLE_H_

#include <imgui/imgui.h>

namespace animtool
{

inline void apply_tool_style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding    = 6.0f;
	style.FrameRounding     = 4.0f;
	style.WindowBorderSize  = 1.0f;
	style.WindowPadding     = ImVec2( 12.0f, 10.0f );
	style.FramePadding      = ImVec2( 8.0f, 4.0f );
	style.ItemSpacing       = ImVec2( 8.0f, 6.0f );

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg]         = ImVec4( 0.05f, 0.06f, 0.09f, 1.00f );
	colors[ImGuiCol_ChildBg]          = ImVec4( 0.04f, 0.05f, 0.08f, 1.00f );
	colors[ImGuiCol_PopupBg]          = ImVec4( 0.05f, 0.06f, 0.09f, 0.98f );
	colors[ImGuiCol_MenuBarBg]        = ImVec4( 0.04f, 0.05f, 0.08f, 1.00f );
	colors[ImGuiCol_TitleBg]          = ImVec4( 0.04f, 0.05f, 0.08f, 1.00f );
	colors[ImGuiCol_TitleBgActive]    = ImVec4( 0.07f, 0.09f, 0.13f, 1.00f );
	colors[ImGuiCol_FrameBg]          = ImVec4( 0.08f, 0.10f, 0.13f, 1.00f );
	colors[ImGuiCol_FrameBgHovered]   = ImVec4( 0.11f, 0.13f, 0.17f, 1.00f );
	colors[ImGuiCol_Button]           = ImVec4( 0.18f, 0.47f, 0.71f, 1.00f );
	colors[ImGuiCol_ButtonHovered]    = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );
	colors[ImGuiCol_Text]             = ImVec4( 0.92f, 0.94f, 0.96f, 1.00f );
	colors[ImGuiCol_Border]           = ImVec4( 0.12f, 0.14f, 0.18f, 1.00f );
}

}

#endif // _ANIMTOOL_UI_STYLE_H_
