#include "HudRenderSystem.h"

#include <imgui/imgui.h>

#include <ecs/ECS.h>
#include <ecs/components/Hud.h>
#include <graphics/ImGuiRenderPass.h>

using namespace eage::ecs;
using namespace eage::graphics;

namespace
{
	ImVec2 resolve_anchor( const glm::vec2& norm_pos, const glm::vec2& offset_px,
						   HudAnchor anchor, const ImVec2& viewport_size,
						   const ImVec2& widget_size )
	{
		float base_x = norm_pos.x * viewport_size.x + offset_px.x;
		float base_y = norm_pos.y * viewport_size.y + offset_px.y;

		switch( anchor )
		{
		case HudAnchor::TOP_LEFT:
			break;
		case HudAnchor::TOP_CENTER:
			base_x -= widget_size.x * 0.5f;
			break;
		case HudAnchor::TOP_RIGHT:
			base_x -= widget_size.x;
			break;
		case HudAnchor::CENTER_LEFT:
			base_y -= widget_size.y * 0.5f;
			break;
		case HudAnchor::CENTER:
			base_x -= widget_size.x * 0.5f;
			base_y -= widget_size.y * 0.5f;
			break;
		case HudAnchor::CENTER_RIGHT:
			base_x -= widget_size.x;
			base_y -= widget_size.y * 0.5f;
			break;
		case HudAnchor::BOTTOM_LEFT:
			base_y -= widget_size.y;
			break;
		case HudAnchor::BOTTOM_CENTER:
			base_x -= widget_size.x * 0.5f;
			base_y -= widget_size.y;
			break;
		case HudAnchor::BOTTOM_RIGHT:
			base_x -= widget_size.x;
			base_y -= widget_size.y;
			break;
		}

		return ImVec2( base_x, base_y );
	}
}

HudRenderSystem::HudRenderSystem( ECSRegistry& registry, ImGuiRenderPass& imgui_pass, float scale_factor )
	: mRegistry( registry )
	, mImGuiPass( imgui_pass )
	, mScaleFactor( scale_factor )
{
	mImGuiPass.AddOverlayCallback( [this]() { Render(); } );
}

void
HudRenderSystem::Render()
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	const ImGuiViewport* vp = ImGui::GetMainViewport();
	ImVec2 viewport_size = vp->Size;

	for( const auto& [entity, hud_tf] : mRegistry.GetComponentMap<HudTransformComponent>() )
	{
		if( !hud_tf.visible )
		{
			continue;
		}

		if( mRegistry.HasComponent<HudTextComponent>( entity ) )
		{
			const auto& text_cmp = mRegistry.GetComponent<HudTextComponent>( entity );
			ImFont* font = mImGuiPass.GetFont( text_cmp.font_size );

			ImVec2 text_size = font->CalcTextSizeA(
				font->FontSize, FLT_MAX, 0.0f, text_cmp.text.c_str() );

			ImVec2 screen_pos = resolve_anchor(
				hud_tf.position, hud_tf.offset_px * mScaleFactor, hud_tf.anchor,
				viewport_size, text_size );

			ImU32 color = ImGui::ColorConvertFloat4ToU32( ImVec4(
				text_cmp.color.r, text_cmp.color.g,
				text_cmp.color.b, text_cmp.color.a ) );

			draw_list->AddText( font, font->FontSize, screen_pos, color,
				text_cmp.text.c_str() );
		}
	}
}
