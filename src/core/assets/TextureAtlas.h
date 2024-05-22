#ifndef _EAGE_TEXTURE_ATLAS_H_
#define _EAGE_TEXTURE_ATLAS_H_

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace assets
{
	class TextureAtlas
	{
	public:
		TextureAtlas( const std::string& atlas_json_path );
		virtual ~TextureAtlas();

		struct SubTexture
		{
			std::string name;
			glm::vec2 uv_min;
			glm::vec2 uv_max;
		};
		const SubTexture& GetSubTexture( const std::string& name ) const;

	private:
		SubTexture mDefaultSubTexture;
		std::unordered_map<std::string, SubTexture> mSubTextures;
	};

}

#endif // _EAGE_TEXTURE_ATLAS_H_