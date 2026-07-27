#ifndef _EAGE_TEXTURE_LOAD_DELEGATE_H_
#define _EAGE_TEXTURE_LOAD_DELEGATE_H_

#include <assets/SceneResourceLoader.h>

namespace eage::ecs
{
	class RenderSystem;

	class TextureLoadDelegate : public assets::SceneResourceLoader::Delegate
	{
	public:
		static constexpr const char* SECTION_KEY = assets::SceneResourceLoader::SECTION_TEXTURES;

		explicit TextureLoadDelegate( RenderSystem& render_system );

		bool Load( const rapidjson::Value& section,
				   assets::SceneResourceLoader::ResourceTable& out ) override;

	private:
		RenderSystem& mRenderSystem;
	};
}

#endif // _EAGE_TEXTURE_LOAD_DELEGATE_H_
