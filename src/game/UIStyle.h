#ifndef _UI_STYLE_H
#define _UI_STYLE_H

#include <imgui/imgui.h>

namespace vortex
{

inline void apply_game_style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// --- Rounding & spacing ---
	style.WindowRounding    = 6.0f;
	style.ChildRounding     = 4.0f;
	style.FrameRounding     = 4.0f;
	style.PopupRounding     = 4.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding      = 4.0f;
	style.TabRounding       = 4.0f;

	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize  = 1.0f;
	style.PopupBorderSize  = 1.0f;

	style.WindowPadding     = ImVec2( 12.0f, 10.0f );
	style.FramePadding      = ImVec2( 8.0f,  4.0f  );
	style.CellPadding       = ImVec2( 4.0f,  4.0f  );
	style.ItemSpacing       = ImVec2( 8.0f,  6.0f  );
	style.ItemInnerSpacing  = ImVec2( 6.0f,  4.0f  );
	style.IndentSpacing     = 18.0f;
	style.ScrollbarSize     = 12.0f;
	style.GrabMinSize       = 10.0f;

	// --- Color palette ---
	// Background: deep navy
	// Accent:     cyan  (0.00, 0.83, 1.00)
	// Accent dim: steel blue (0.18, 0.47, 0.71)
	// Warning:    amber (1.00, 0.75, 0.00)
	// Text:       near-white

	ImVec4* c = style.Colors;

	c[ImGuiCol_Text]                  = ImVec4( 0.90f, 0.95f, 1.00f, 1.00f );
	c[ImGuiCol_TextDisabled]          = ImVec4( 0.40f, 0.45f, 0.55f, 1.00f );

	c[ImGuiCol_WindowBg]              = ImVec4( 0.04f, 0.04f, 0.08f, 0.92f );
	c[ImGuiCol_ChildBg]               = ImVec4( 0.02f, 0.03f, 0.06f, 0.80f );
	c[ImGuiCol_PopupBg]               = ImVec4( 0.06f, 0.07f, 0.12f, 0.95f );

	c[ImGuiCol_Border]                = ImVec4( 0.00f, 0.60f, 0.80f, 0.40f );
	c[ImGuiCol_BorderShadow]          = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );

	c[ImGuiCol_FrameBg]               = ImVec4( 0.07f, 0.09f, 0.15f, 1.00f );
	c[ImGuiCol_FrameBgHovered]        = ImVec4( 0.10f, 0.14f, 0.22f, 1.00f );
	c[ImGuiCol_FrameBgActive]         = ImVec4( 0.00f, 0.50f, 0.70f, 0.60f );

	c[ImGuiCol_TitleBg]               = ImVec4( 0.02f, 0.04f, 0.10f, 1.00f );
	c[ImGuiCol_TitleBgActive]         = ImVec4( 0.00f, 0.30f, 0.45f, 1.00f );
	c[ImGuiCol_TitleBgCollapsed]      = ImVec4( 0.02f, 0.04f, 0.10f, 0.75f );

	c[ImGuiCol_MenuBarBg]             = ImVec4( 0.03f, 0.04f, 0.09f, 1.00f );

	c[ImGuiCol_ScrollbarBg]           = ImVec4( 0.02f, 0.02f, 0.05f, 0.80f );
	c[ImGuiCol_ScrollbarGrab]         = ImVec4( 0.00f, 0.50f, 0.70f, 0.80f );
	c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4( 0.00f, 0.70f, 0.90f, 1.00f );
	c[ImGuiCol_ScrollbarGrabActive]   = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_CheckMark]             = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_SliderGrab]            = ImVec4( 0.00f, 0.65f, 0.85f, 1.00f );
	c[ImGuiCol_SliderGrabActive]      = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_Button]                = ImVec4( 0.00f, 0.40f, 0.60f, 0.80f );
	c[ImGuiCol_ButtonHovered]         = ImVec4( 0.00f, 0.60f, 0.80f, 1.00f );
	c[ImGuiCol_ButtonActive]          = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_Header]                = ImVec4( 0.00f, 0.40f, 0.60f, 0.60f );
	c[ImGuiCol_HeaderHovered]         = ImVec4( 0.00f, 0.60f, 0.80f, 0.80f );
	c[ImGuiCol_HeaderActive]          = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_Separator]             = ImVec4( 0.00f, 0.50f, 0.70f, 0.50f );
	c[ImGuiCol_SeparatorHovered]      = ImVec4( 0.00f, 0.70f, 0.90f, 0.80f );
	c[ImGuiCol_SeparatorActive]       = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_ResizeGrip]            = ImVec4( 0.00f, 0.50f, 0.70f, 0.30f );
	c[ImGuiCol_ResizeGripHovered]     = ImVec4( 0.00f, 0.70f, 0.90f, 0.70f );
	c[ImGuiCol_ResizeGripActive]      = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );

	c[ImGuiCol_Tab]                   = ImVec4( 0.03f, 0.06f, 0.12f, 1.00f );
	c[ImGuiCol_TabHovered]            = ImVec4( 0.00f, 0.55f, 0.75f, 1.00f );
	c[ImGuiCol_TabActive]             = ImVec4( 0.00f, 0.40f, 0.60f, 1.00f );
	c[ImGuiCol_TabUnfocused]          = ImVec4( 0.02f, 0.04f, 0.08f, 1.00f );
	c[ImGuiCol_TabUnfocusedActive]    = ImVec4( 0.05f, 0.12f, 0.22f, 1.00f );

	c[ImGuiCol_PlotLines]             = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );
	c[ImGuiCol_PlotLinesHovered]      = ImVec4( 1.00f, 0.75f, 0.00f, 1.00f );
	c[ImGuiCol_PlotHistogram]         = ImVec4( 0.00f, 0.65f, 0.85f, 1.00f );
	c[ImGuiCol_PlotHistogramHovered]  = ImVec4( 1.00f, 0.75f, 0.00f, 1.00f );

	c[ImGuiCol_TableHeaderBg]         = ImVec4( 0.03f, 0.07f, 0.14f, 1.00f );
	c[ImGuiCol_TableBorderStrong]     = ImVec4( 0.00f, 0.50f, 0.70f, 0.60f );
	c[ImGuiCol_TableBorderLight]      = ImVec4( 0.00f, 0.35f, 0.50f, 0.40f );
	c[ImGuiCol_TableRowBg]            = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_TableRowBgAlt]         = ImVec4( 0.07f, 0.10f, 0.16f, 0.40f );

	c[ImGuiCol_TextSelectedBg]        = ImVec4( 0.00f, 0.50f, 0.70f, 0.50f );
	c[ImGuiCol_DragDropTarget]        = ImVec4( 0.00f, 0.83f, 1.00f, 0.90f );
	c[ImGuiCol_NavHighlight]          = ImVec4( 0.00f, 0.83f, 1.00f, 1.00f );
	c[ImGuiCol_NavWindowingHighlight] = ImVec4( 0.00f, 0.83f, 1.00f, 0.70f );
	c[ImGuiCol_NavWindowingDimBg]     = ImVec4( 0.00f, 0.00f, 0.00f, 0.50f );
	c[ImGuiCol_ModalWindowDimBg]      = ImVec4( 0.00f, 0.00f, 0.00f, 0.60f );
}

} // namespace vortex

#endif // _UI_STYLE_H
