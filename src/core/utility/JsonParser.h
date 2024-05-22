#ifndef _EAGE_JSON_PARSER_H_
#define _EAGE_JSON_PARSER_H_

#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

#include <utility/Logger.h>

namespace utility
{
	static const rapidjson::Value empty_json_value;
	bool parse_json_document( rapidjson::Document& doc, const std::string& file_path )
	{
		std::ifstream ifs( file_path );
		if( !ifs.is_open() )
		{
			LOG_ERROR( "Failed to open file: " + file_path );
			return false;
		}

		rapidjson::IStreamWrapper isw( ifs );
		doc.ParseStream( isw );

		if( doc.HasParseError() )
		{
			LOG_ERROR( "Failed to parse json: " + file_path );
			LOG_ERROR( "Error: " + std::to_string( doc.GetParseError() ) );
			return false;
		}

		return true;
	}

	const rapidjson::Value& get_json_object( const rapidjson::Value& parent, const std::string& key )
	{
		if( !parent.HasMember( key.c_str() ) )
		{
			LOG_ERROR( "Expected json object." );
			return empty_json_value;
		}

		return parent[key.c_str()];
	}

	int get_json_int( const rapidjson::Value& parent, const std::string& key )
	{
		if( !parent.HasMember( key.c_str() ) || !parent[key.c_str()].IsInt() )
		{
			LOG_ERROR( "Expected json int." );
			return 0;
		}

		return parent[key.c_str()].GetInt();
	}

	const rapidjson::Value& get_json_array( const rapidjson::Value& parent, const std::string& key )
	{
		if( !parent.HasMember( key.c_str() ) || !parent[key.c_str()].IsArray() )
		{
			LOG_ERROR( "Expected json array." );
			return empty_json_value;
		}

		return parent[key.c_str()].GetArray();
	}

	std::string get_json_string( const rapidjson::Value& parent, const std::string& key )
	{
		if( !parent.HasMember( key.c_str() ) || !parent[key.c_str()].IsString() )
		{
			LOG_ERROR( "Expected json string." );
			return "";
		}

		return parent[key.c_str()].GetString();
	}
}

#endif // _EAGE_JSON_PARSER_H_