#ifndef _EAGE_ABSTRACT_HUD_RENDERER_H_
#define _EAGE_ABSTRACT_HUD_RENDERER_H_

#include <functional>
#include <string>

#include <glm/glm.hpp>

#include <ecs/components/Hud.h>

namespace eage::ecs
{
	class AbstractHudRenderer
	{
	public:
		virtual ~AbstractHudRenderer() = default;

		///
		/// Register a callback to be invoked each frame during HUD rendering
		///
		virtual void RegisterRenderCallback( std::function<void()> callback ) = 0;

		///
		/// Returns the current viewport size in pixels
		///
		virtual glm::vec2 GetViewportSize() const = 0;

		///
		/// Measures the pixel dimensions of a text string for a given font slot
		///
		virtual glm::vec2 MeasureText( const std::string& text, HudFontSize size ) const = 0;

		///
		/// Draws text at a screen-space pixel position
		///
		virtual void DrawText( glm::vec2 screen_pos, const std::string& text, glm::vec4 color, HudFontSize size ) = 0;
	};
}

#endif // _EAGE_ABSTRACT_HUD_RENDERER_H_
