#include "EnemyDefinition.h"

#include <utility>

#include <utility/JsonParser.h>
#include <utility/Logger.h>

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
	optional_float( const rapidjson::Value& parent, const char* key, const std::string& path, float& out )
	{
		if( !parent.HasMember( key ) )
		{
			return true;
		}
		if( !parent[key].IsNumber() )
		{
			log_field_error( path, key, "expected number" );
			return false;
		}
		out = parent[key].GetFloat();
		return true;
	}

	bool
	optional_int( const rapidjson::Value& parent, const char* key, const std::string& path, int& out )
	{
		if( !parent.HasMember( key ) )
		{
			return true;
		}
		if( !parent[key].IsInt() )
		{
			log_field_error( path, key, "expected int" );
			return false;
		}
		out = parent[key].GetInt();
		return true;
	}

	bool
	optional_bool( const rapidjson::Value& parent, const char* key, const std::string& path, bool& out )
	{
		if( !parent.HasMember( key ) )
		{
			return true;
		}
		if( !parent[key].IsBool() )
		{
			log_field_error( path, key, "expected bool" );
			return false;
		}
		out = parent[key].GetBool();
		return true;
	}

	bool
	load_drift_params( const rapidjson::Value& parent, const std::string& path, DriftParams& out )
	{
		if( !parent.HasMember( "drift" ) || !parent["drift"].IsObject() )
		{
			log_field_error( path, "drift", "expected object" );
			return false;
		}

		const rapidjson::Value& drift = parent["drift"];
		const std::string drift_path = path + ":drift";
		if( !optional_float( drift, "speed_min", drift_path, out.speed_min ) ||
			!optional_float( drift, "speed_max", drift_path, out.speed_max ) ||
			!optional_float( drift, "angular_speed_max", drift_path, out.angular_speed_max ) )
		{
			return false;
		}

		return true;
	}
}

bool
vortex::parse_enemy_behavior( const std::string& name, EnemyBehavior& out )
{
	if( name == "drift" )
	{
		out = EnemyBehavior::DRIFT;
		return true;
	}

	return false;
}

bool
vortex::load_enemy_definition( const std::string& path, EnemyDefinition& out )
{
	rapidjson::Document document;
	if( !parse_json_document( document, path ) )
	{
		return false;
	}

	if( !require_object_root( document, path ) )
	{
		return false;
	}

	EnemyDefinition loaded;

	if( !require_int( document, "version", path, loaded.version ) )
	{
		return false;
	}
	if( loaded.version != 1 )
	{
		log_field_error( path, "version", "unsupported version" );
		return false;
	}

	if( !require_string( document, "id", path, loaded.id ) )
	{
		return false;
	}
	if( loaded.id.empty() )
	{
		log_field_error( path, "id", "must not be empty" );
		return false;
	}

	std::string behavior_name;
	if( !require_string( document, "behavior", path, behavior_name ) )
	{
		return false;
	}
	if( !parse_enemy_behavior( behavior_name, loaded.behavior ) )
	{
		log_field_error( path, "behavior", "unknown behavior" );
		return false;
	}

	if( !require_string( document, "texture", path, loaded.texture_path ) )
	{
		return false;
	}
	if( loaded.texture_path.empty() )
	{
		log_field_error( path, "texture", "must not be empty" );
		return false;
	}

	if( !optional_float( document, "sprite_width", path, loaded.sprite_width ) ||
		!optional_float( document, "sprite_height", path, loaded.sprite_height ) ||
		!optional_float( document, "collider_radius", path, loaded.collider_radius ) ||
		!optional_float( document, "max_health", path, loaded.max_health ) ||
		!optional_float( document, "contact_damage", path, loaded.contact_damage ) ||
		!optional_float( document, "max_linear_velocity", path, loaded.max_linear_velocity ) ||
		!optional_int( document, "xp_reward", path, loaded.xp_reward ) ||
		!optional_bool( document, "warpable", path, loaded.warpable ) )
	{
		return false;
	}

	if( loaded.behavior == EnemyBehavior::DRIFT )
	{
		if( !load_drift_params( document, path, loaded.drift ) )
		{
			return false;
		}
	}

	out = std::move( loaded );
	return true;
}
