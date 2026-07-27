#ifndef _EAGE_AUDIO_LOAD_DELEGATE_H_
#define _EAGE_AUDIO_LOAD_DELEGATE_H_

#include <assets/SceneResourceLoader.h>

namespace eage::ecs
{
	class AudioSystem;

	class AudioLoadDelegate : public assets::SceneResourceLoader::Delegate
	{
	public:
		static constexpr const char* SECTION_KEY = assets::SceneResourceLoader::SECTION_SOUNDS;

		explicit AudioLoadDelegate( AudioSystem& audio_system );

		bool Load( const rapidjson::Value& section,
				   assets::SceneResourceLoader::ResourceTable& out ) override;

	private:
		AudioSystem& mAudioSystem;
	};
}

#endif // _EAGE_AUDIO_LOAD_DELEGATE_H_
