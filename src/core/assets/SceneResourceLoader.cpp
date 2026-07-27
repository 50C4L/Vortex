#include "SceneResourceLoader.h"

#include <utility/Logger.h>

using namespace assets;
using namespace utility;

void
SceneResourceLoader::RegisterDelegate( const std::string& section_key, Delegate& delegate )
{
	mDelegates[section_key] = &delegate;
}

bool
SceneResourceLoader::LoadManifest( const std::string& manifest_path )
{
	rapidjson::Document document;
	if( !parse_json_document( document, manifest_path ) )
	{
		LOG_ERROR() << "SceneResourceLoader: failed to parse manifest " << manifest_path;
		return false;
	}

	if( !document.IsObject() )
	{
		LOG_ERROR() << "SceneResourceLoader: manifest root must be an object: " << manifest_path;
		return false;
	}

	for( auto it = document.MemberBegin(); it != document.MemberEnd(); ++it )
	{
		const std::string section_key = it->name.GetString();
		auto delegate_it = mDelegates.find( section_key );
		if( delegate_it == mDelegates.end() )
		{
			LOG_ERROR() << "SceneResourceLoader: no delegate for section " << section_key;
			continue;
		}

		if( !delegate_it->second->Load( it->value, mTables[section_key] ) )
		{
			LOG_ERROR() << "SceneResourceLoader: delegate failed for section " << section_key;
		}
	}

	return true;
}

uint32_t
SceneResourceLoader::GetTexture( const std::string& path ) const
{
	return Lookup( SECTION_TEXTURES, path );
}

uint32_t
SceneResourceLoader::GetClip( const std::string& path ) const
{
	return Lookup( SECTION_ANIMATIONS, path );
}

uint32_t
SceneResourceLoader::GetSound( const std::string& path ) const
{
	return Lookup( SECTION_SOUNDS, path );
}

uint32_t
SceneResourceLoader::Lookup( const std::string& section_key, const std::string& path ) const
{
	auto table_it = mTables.find( section_key );
	if( table_it == mTables.end() )
	{
		LOG_ERROR() << "SceneResourceLoader: section not loaded: " << section_key;
		return 0;
	}

	auto it = table_it->second.find( path );
	if( it == table_it->second.end() )
	{
		LOG_ERROR() << "SceneResourceLoader: " << section_key << " not in catalog: " << path;
		return 0;
	}

	return it->second;
}
