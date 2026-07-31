#ifndef _EAGE_UI_VIEW_H_
#define _EAGE_UI_VIEW_H_

#include <cstdint>
#include <memory>
#include <string>

#include <ui/UIDataModel.h>

namespace eage::graphics
{
	class AbstractRenderPass;
	struct ManagedImage;
}

namespace eage::ui
{
	class UISystem;

	/// Scene-lifetime RmlUi context + document + UI offscreen render pass.
	class UIView
	{
	public:
		UIView(
			UISystem& system,
			const std::string& name,
			uint32_t width,
			uint32_t height,
			const std::string& model_name = "hud" );
		~UIView();

		UIView( const UIView& ) = delete;
		UIView& operator=( const UIView& ) = delete;

		UIDataModel& GetDataModel();

		void BindImage( const std::string& name, graphics::ManagedImage& image );
		bool LoadDocument( const std::string& rml_path );

		graphics::AbstractRenderPass& GetRenderPass();
		graphics::ManagedImage* GetOutput();

	private:
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	};
}

#endif // _EAGE_UI_VIEW_H_
