#include "Filesystem.h"

#include <vector>

#include <utility/Logger.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#endif

namespace utility
{

bool
is_file_exist( const std::string& filename )
{
	return std::filesystem::exists( std::filesystem::path{ filename } );
}

std::filesystem::path
get_executable_directory()
{
#ifdef _WIN32
	wchar_t path[MAX_PATH]{};
	const DWORD length = GetModuleFileNameW( nullptr, path, MAX_PATH );
	if( length == 0 || length == MAX_PATH )
	{
		return std::filesystem::current_path();
	}
	return std::filesystem::path( path ).parent_path();
#else
	std::error_code ec;
	auto exe_path = std::filesystem::read_symlink( "/proc/self/exe", ec );
	if( ec )
	{
		return std::filesystem::current_path();
	}
	return exe_path.parent_path();
#endif
}

void
init_content_working_directory()
{
	const std::filesystem::path exe_dir = get_executable_directory();
	const std::vector<std::filesystem::path> candidates = {
		exe_dir,
		std::filesystem::current_path(),
		exe_dir / ".." / "..",
		exe_dir / "..",
	};

	for( const auto& candidate : candidates )
	{
		std::error_code ec;
		const std::filesystem::path normalized = std::filesystem::weakly_canonical( candidate, ec );
		if( ec )
		{
			continue;
		}

		if( std::filesystem::is_directory( normalized / "resources" ) )
		{
			std::filesystem::current_path( normalized );
			LOG( "Content root: " + normalized.string() );
			return;
		}
	}

	LOG_ERROR( "Could not locate resources directory near executable or workspace" );
}

}
