#include "HudRenderSystem.h"
#include "AbstractHudRenderer.h"

#include <glm/glm.hpp>

#include <ecs/ECS.h>
#include <ecs/components/Hud.h>

using namespace eage::ecs;

namespace
{
	glm::vec2 resolve_anchor( const glm::vec2& norm_pos, const glm::vec2& offset_px,
						   HudAnchor anchor, const glm::vec2& viewport_size,
						   const glm::vec2& widget_size )
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

		return glm::vec2( base_x, base_y );
	}
}

HudRenderSystem::HudRenderSystem( ECSRegistry& registry, AbstractHudRenderer& hud_renderer, float scale_factor )
	: mRegistry( registry )
	, mHudRenderer( hud_renderer )
	, mScaleFactor( scale_factor )
{
	mHudRenderer.RegisterRenderCallback( [this]() { Render(); } );
}

void
HudRenderSystem::Render()
{
	glm::vec2 viewport_size = mHudRenderer.GetViewportSize();

	for( auto [entity, hud_tf] : mRegistry.GetComponentMap<HudTransformComponent>() )
	{
		if( !hud_tf.visible )
		{
			continue;
		}

		if( mRegistry.HasComponent<HudTextComponent>( entity ) )
		{
			const auto& text_cmp = mRegistry.GetComponent<HudTextComponent>( entity );

			glm::vec2 text_size = mHudRenderer.MeasureText( text_cmp.text, text_cmp.font_size );

			glm::vec2 screen_pos = resolve_anchor(
				hud_tf.position, hud_tf.offset_px * mScaleFactor, hud_tf.anchor,
				viewport_size, text_size );

			mHudRenderer.DrawText( screen_pos, text_cmp.text, text_cmp.color, text_cmp.font_size );
		}
	}
}
