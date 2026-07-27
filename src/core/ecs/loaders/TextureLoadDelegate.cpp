#include "TextureLoadDelegate.h"

#include <ecs/systems/RenderSystem.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace utility;

TextureLoadDelegate::TextureLoadDelegate( RenderSystem& render_system )
	: mRenderSystem( render_system )
{
}

bool
TextureLoadDelegate::Load( const rapidjson::Value& section,
						   assets::SceneResourceLoader::ResourceTable& out )
{
	if( !section.IsArray() )
	{
		LOG_ERROR() << "TextureLoadDelegate: section must be an array";
		return false;
	}

	bool ok = true;
	for( const auto& texture_val : section.GetArray() )
	{
		if( !texture_val.IsString() )
		{
			LOG_ERROR() << "TextureLoadDelegate: texture entry must be a string path";
			ok = false;
			continue;
		}

		const std::string path = texture_val.GetString();
		try
		{
			const uint32_t texture_index = mRenderSystem.CreateTexture( path );
			out[path] = texture_index;
		}
		catch( const std::exception& ex )
		{
			LOG_ERROR() << "TextureLoadDelegate: failed to load texture " << path << ": " << ex.what();
			ok = false;
		}
	}

	return ok;
}
