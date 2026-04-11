#ifndef _EAGE_HUD_RENDER_SYSTEM_H_
#define _EAGE_HUD_RENDER_SYSTEM_H_

namespace eage::ecs
{
	class ECSRegistry;
}

namespace eage::graphics
{
	class ImGuiRenderPass;
}

namespace eage::ecs
{
	class HudRenderSystem
	{
	public:
		HudRenderSystem( ECSRegistry& registry, eage::graphics::ImGuiRenderPass& imgui_pass, float scale_factor );

		void Render();

	private:
		ECSRegistry& mRegistry;
		eage::graphics::ImGuiRenderPass& mImGuiPass;
		float mScaleFactor;
	};
}

#endif // _EAGE_HUD_RENDER_SYSTEM_H_
