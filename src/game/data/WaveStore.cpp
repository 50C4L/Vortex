#include "WaveStore.h"

#include <string>
#include <utility>

#include <utility/JsonParser.h>
#include <utility/Logger.h>

#include "EnemyDefinition.h"

using namespace vortex;
using namespace utility;

namespace
{
	void
	log_field_error( const std::string& path, const char* field, const char* message )
	{
		LOG_ERROR() << path << ":" << field << ": " << message;
	}

	bool
	require_object_root( const rapidjson::Document& doc, const std::string& path )
	{
		if( !doc.IsObject() )
		{
			LOG_ERROR() << path << ": root must be an object";
			return false;
		}
		return true;
	}

	bool
	require_int( const rapidjson::Value& parent, const char* key, const std::string& path, int& out )
	{
		if( !parent.HasMember( key ) || !parent[key].IsInt() )
		{
			log_field_error( path, key, "expected int" );
			return false;
		}
		out = parent[key].GetInt();
		return true;
	}

	bool
	require_number( const rapidjson::Value& parent, const char* key, const std::string& path, float& out )
	{
		if( !parent.HasMember( key ) || !parent[key].IsNumber() )
		{
			log_field_error( path, key, "expected number" );
			return false;
		}
		out = parent[key].GetFloat();
		return true;
	}

	bool
	require_string( const rapidjson::Value& parent, const char* key, const std::string& path, std::string& out )
	{
		if( !parent.HasMember( key ) || !parent[key].IsString() )
		{
			log_field_error( path, key, "expected string" );
			return false;
		}
		out = parent[key].GetString();
		return true;
	}

	bool
	parse_spawn_type( const std::string& name, SpawnType& out )
	{
		if( name == "batch" )
		{
			out = SpawnType::BATCH;
			return true;
		}
		return false;
	}

	bool
	validate_enemy_file( const std::string& id, const std::string& wave_path )
	{
		const std::string enemy_path = "./resources/enemies/" + id + ".json";
		EnemyDefinition definition;
		if( !load_enemy_definition( enemy_path, definition ) )
		{
			log_field_error( wave_path, "id", "failed to load enemy json" );
			return false;
		}
		if( definition.id != id )
		{
			log_field_error( wave_path, "id", "enemy json id does not match filename" );
			return false;
		}
		return true;
	}

	bool
	load_spawn_group( const rapidjson::Value& value, const std::string& path, SpawnGroup& out )
	{
		if( !value.IsObject() )
		{
			LOG_ERROR() << path << ": expected object";
			return false;
		}

		SpawnGroup group;
		if( !require_string( value, "id", path, group.id ) )
		{
			return false;
		}
		if( group.id.empty() )
		{
			log_field_error( path, "id", "must not be empty" );
			return false;
		}

		if( !require_int( value, "total", path, group.total ) )
		{
			return false;
		}
		if( group.total <= 0 )
		{
			log_field_error( path, "total", "must be greater than 0" );
			return false;
		}

		std::string spawn_type_name;
		if( !require_string( value, "spawn_type", path, spawn_type_name ) )
		{
			return false;
		}
		if( !parse_spawn_type( spawn_type_name, group.spawn_type ) )
		{
			log_field_error( path, "spawn_type", "unknown spawn type" );
			return false;
		}

		if( !require_int( value, "batch_size", path, group.batch_size ) )
		{
			return false;
		}
		if( group.batch_size <= 0 )
		{
			log_field_error( path, "batch_size", "must be greater than 0" );
			return false;
		}
		if( group.batch_size > group.total )
		{
			log_field_error( path, "batch_size", "must not exceed total" );
			return false;
		}

		if( !require_number( value, "spawn_interval", path, group.spawn_interval ) )
		{
			return false;
		}
		if( group.spawn_interval < 0.f )
		{
			log_field_error( path, "spawn_interval", "must not be negative" );
			return false;
		}

		if( !validate_enemy_file( group.id, path ) )
		{
			return false;
		}

		out = std::move( group );
		return true;
	}

	bool
	load_wave( const rapidjson::Value& value, const std::string& path, WaveDefinition& out )
	{
		if( !value.IsObject() )
		{
			LOG_ERROR() << path << ": expected object";
			return false;
		}

		WaveDefinition wave;
		if( !require_number( value, "time_sec", path, wave.time_sec ) )
		{
			return false;
		}
		if( wave.time_sec <= 0.f )
		{
			log_field_error( path, "time_sec", "must be greater than 0" );
			return false;
		}

		if( !value.HasMember( "enemies" ) || !value["enemies"].IsArray() )
		{
			log_field_error( path, "enemies", "expected array" );
			return false;
		}

		const rapidjson::Value& enemies = value["enemies"];
		if( enemies.Size() == 0 )
		{
			log_field_error( path, "enemies", "must not be empty" );
			return false;
		}

		wave.enemies.reserve( enemies.Size() );
		for( rapidjson::SizeType i = 0; i < enemies.Size(); ++i )
		{
			const std::string group_path = path + ":enemies[" + std::to_string( i ) + "]";
			SpawnGroup group;
			if( !load_spawn_group( enemies[i], group_path, group ) )
			{
				return false;
			}
			wave.enemies.push_back( std::move( group ) );
		}

		out = std::move( wave );
		return true;
	}
}

bool
WaveStore::Load( const std::string& path )
{
	mWaves.clear();

	rapidjson::Document document;
	if( !parse_json_document( document, path ) )
	{
		return false;
	}

	if( !require_object_root( document, path ) )
	{
		return false;
	}

	int version = 0;
	if( !require_int( document, "version", path, version ) )
	{
		return false;
	}
	if( version != 1 )
	{
		log_field_error( path, "version", "unsupported version" );
		return false;
	}

	if( !document.HasMember( "waves" ) || !document["waves"].IsArray() )
	{
		log_field_error( path, "waves", "expected array" );
		return false;
	}

	const rapidjson::Value& waves = document["waves"];
	if( waves.Size() == 0 )
	{
		log_field_error( path, "waves", "must not be empty" );
		return false;
	}

	std::vector<WaveDefinition> loaded;
	loaded.reserve( waves.Size() );
	for( rapidjson::SizeType i = 0; i < waves.Size(); ++i )
	{
		const std::string wave_path = path + ":waves[" + std::to_string( i ) + "]";
		WaveDefinition wave;
		if( !load_wave( waves[i], wave_path, wave ) )
		{
			return false;
		}
		loaded.push_back( std::move( wave ) );
	}

	mWaves = std::move( loaded );
	return true;
}

const std::vector<WaveDefinition>&
WaveStore::GetWaves() const
{
	return mWaves;
}

std::unordered_map<std::string, int>
WaveStore::ComputePoolRequirements() const
{
	std::unordered_map<std::string, int> requirements;
	for( const WaveDefinition& wave : mWaves )
	{
		std::unordered_map<std::string, int> wave_totals;
		for( const SpawnGroup& group : wave.enemies )
		{
			wave_totals[group.id] += group.total;
		}
		for( const auto& [id, total] : wave_totals )
		{
			auto it = requirements.find( id );
			if( it == requirements.end() || total > it->second )
			{
				requirements[id] = total;
			}
		}
	}
	return requirements;
}
