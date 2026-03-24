#ifndef _EAGE_COMPONENTS_HUD_H_
#define _EAGE_COMPONENTS_HUD_H_

#include <string>
#include <glm/glm.hpp>

namespace eage::ecs
{
	enum class HudAnchor
	{
		TOP_LEFT,
		TOP_CENTER,
		TOP_RIGHT,
		CENTER_LEFT,
		CENTER,
		CENTER_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_CENTER,
		BOTTOM_RIGHT
	};

	enum class HudFontSize
	{
		SMALL,
		MEDIUM,
		LARGE,
		COUNT
	};

	struct HudTransformComponent
	{
		glm::vec2 position = glm::vec2( 0.0f );     // Normalized 0-1 (0,0 = top-left)
		glm::vec2 offset_px = glm::vec2( 0.0f );    // Pixel nudge after anchoring
		HudAnchor anchor = HudAnchor::TOP_LEFT;
		bool visible = true;
	};

	struct HudTextComponent
	{
		std::string text;
		glm::vec4 color = glm::vec4( 1.0f );        // RGBA
		HudFontSize font_size = HudFontSize::MEDIUM;
	};

	struct HudBarComponent
	{
		float current = 1.0f;
		float max = 1.0f;
		glm::vec2 size_px = glm::vec2( 200.0f, 20.0f );
		glm::vec4 fill_color = glm::vec4( 0.0f, 0.83f, 1.0f, 1.0f );
		glm::vec4 bg_color = glm::vec4( 0.1f, 0.1f, 0.15f, 0.8f );
	};
}

#endif // _EAGE_COMPONENTS_HUD_H_
