#ifndef _EAGE_FILESYSTEM_H_
#define _EAGE_FILESYSTEM_H_

#include <string>
#include <filesystem>

namespace utility
{
	bool is_file_exist( const std::string& filename );

	std::filesystem::path get_executable_directory();
	void init_content_working_directory();
}

#endif // _EAGE_FILESYSTEM_H_