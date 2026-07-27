#ifndef _EAGE_FONT_LOAD_DELEGATE_H_
#define _EAGE_FONT_LOAD_DELEGATE_H_

#include <assets/SceneResourceLoader.h>

namespace eage::ui
{
	class UISystem;

	///
	/// Loads RmlUi font faces from a manifest "fonts" section.
	/// Leaves the ResourceTable empty - font faces are global and produce no handle.
	///
	class FontLoadDelegate : public assets::SceneResourceLoader::Delegate
	{
	public:
		static constexpr const char* SECTION_KEY = "fonts";

		explicit FontLoadDelegate( UISystem& system );

		bool Load( const rapidjson::Value& section,
				   assets::SceneResourceLoader::ResourceTable& out ) override;

	private:
		UISystem& mUISystem;
	};
}

#endif // _EAGE_FONT_LOAD_DELEGATE_H_
