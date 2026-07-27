#include "FontLoadDelegate.h"

#include <ui/UISystem.h>
#include <utility/Logger.h>

using namespace eage::ui;
using namespace utility;

FontLoadDelegate::FontLoadDelegate( UISystem& system )
	: mUISystem( system )
{
}

bool
FontLoadDelegate::Load( const rapidjson::Value& section,
						assets::SceneResourceLoader::ResourceTable& /*out*/ )
{
	if( !section.IsArray() )
	{
		LOG_ERROR() << "FontLoadDelegate: section must be an array";
		return false;
	}

	bool ok = true;
	for( const auto& font_val : section.GetArray() )
	{
		if( !font_val.IsString() )
		{
			LOG_ERROR() << "FontLoadDelegate: font entry must be a string path";
			ok = false;
			continue;
		}

		const std::string path = font_val.GetString();
		if( !mUISystem.LoadFontFace( path ) )
		{
			LOG_ERROR() << "FontLoadDelegate: failed to load font " << path;
			ok = false;
		}
	}

	return ok;
}
