#ifndef _EAGE_UI_SYSTEM_H_
#define _EAGE_UI_SYSTEM_H_

#include <memory>
#include <string>

namespace eage::graphics
{
	class Renderer;
}

namespace eage::ui
{
	class UIRenderInterface;
	class UISystemInterface;

	/// App-lifetime owner of RmlUi initialise/shutdown and shared interfaces.
	class UISystem
	{
	public:
		explicit UISystem( graphics::Renderer& renderer );
		~UISystem();

		UISystem( const UISystem& ) = delete;
		UISystem& operator=( const UISystem& ) = delete;

		bool LoadFontFace( const std::string& path );

		graphics::Renderer& GetRenderer() { return mRenderer; }

	private:
		friend class UIView;

		graphics::Renderer& mRenderer;
		std::unique_ptr<UISystemInterface> mSystemInterface;
		std::unique_ptr<UIRenderInterface> mRenderInterface;
		bool mInitialised = false;
	};
}

#endif // _EAGE_UI_SYSTEM_H_
