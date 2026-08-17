#ifndef _EAGE_SCENE_RESOURCE_LOADER_H_
#define _EAGE_SCENE_RESOURCE_LOADER_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <utility/JsonParser.h>

namespace assets
{
	///
	/// SceneResourceLoader: Parses a scene manifest JSON and dispatches each
	/// section to a registered Delegate. Acts as a catalog facade: delegates
	/// fill opaque path-to-id tables; typed getters look them up by section.
	///
	class SceneResourceLoader
	{
	public:
		/// Manifest path -> opaque id (bindless texture index or ResourceId).
		using ResourceTable = std::unordered_map<std::string, uint32_t>;

		///
		/// Loads one named manifest section. Implementations live in whichever
		/// module owns the target subsystem.
		///
		class Delegate
		{
		public:
			virtual ~Delegate() = default;

			/// section is the raw JSON node for this delegate's key, since schemas
			/// differ per asset type. Fill out only for addressable resources.
			virtual bool Load( const rapidjson::Value& section, ResourceTable& out ) = 0;
		};

		static constexpr const char* SECTION_TEXTURES = "textures";
		static constexpr const char* SECTION_ANIMATIONS = "animations";
		static constexpr const char* SECTION_SOUNDS = "sounds";

		SceneResourceLoader() = default;

		void RegisterDelegate( const std::string& section_key, Delegate& delegate );
		bool LoadManifest( const std::string& manifest_path );

		/// uint32_t rather than ecs::ResourceId so assets stays free of ecs; the
		/// two are the same type, so call sites are unaffected.
		uint32_t GetTexture( const std::string& path ) const;
		bool HasTexture( const std::string& path ) const;
		uint32_t GetClip( const std::string& path ) const;
		uint32_t GetSound( const std::string& path ) const;

	private:
		uint32_t Lookup( const std::string& section_key, const std::string& path ) const;

		std::unordered_map<std::string, Delegate*> mDelegates;
		std::unordered_map<std::string, ResourceTable> mTables;
	};
}

#endif // _EAGE_SCENE_RESOURCE_LOADER_H_
