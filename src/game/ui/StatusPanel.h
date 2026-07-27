#ifndef _VORTEX_STATUS_PANEL_H_
#define _VORTEX_STATUS_PANEL_H_

namespace eage::ecs
{
	class ECSRegistry;
}

namespace eage::ui
{
	class UIDataModel;
}

namespace vortex
{
	class StatusPanel
	{
	public:
		StatusPanel( eage::ecs::ECSRegistry& registry, eage::ui::UIDataModel& model );

		void Update();

	private:
		eage::ecs::ECSRegistry& mRegistry;
		eage::ui::UIDataModel& mModel;
	};
}

#endif // _VORTEX_STATUS_PANEL_H_
