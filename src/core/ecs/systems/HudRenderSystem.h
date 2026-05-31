#ifndef _EAGE_HUD_RENDER_SYSTEM_H_
#define _EAGE_HUD_RENDER_SYSTEM_H_

namespace eage::ecs
{
	class ECSRegistry;
	class AbstractHudRenderer;
}

namespace eage::ecs
{
	class HudRenderSystem
	{
	public:
		HudRenderSystem( ECSRegistry& registry, AbstractHudRenderer& hud_renderer, float scale_factor );

		void Render();

	private:
		ECSRegistry& mRegistry;
		AbstractHudRenderer& mHudRenderer;
		float mScaleFactor;
	};
}

#endif // _EAGE_HUD_RENDER_SYSTEM_H_
