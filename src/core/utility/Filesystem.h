#ifndef _EAGE_FILESYSTEM_H_
#define _EAGE_FILESYSTEM_H_

#include <string>
#include <filesystem>

namespace utility
{
	bool is_file_exist( const std::string& filename )
	{
		return std::filesystem::exists( std::filesystem::path{ filename } );
	}
}

#endif // _EAGE_FILESYSTEM_H_