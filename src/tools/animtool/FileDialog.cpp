#include "FileDialog.h"

#include <SDL2/SDL_syswm.h>
#include <nfd/nfd.h>

#include <utility/Logger.h>

namespace animtool
{

namespace
{
	std::string normalize_extension( const std::string& extension )
	{
		if( extension.empty() )
		{
			return {};
		}

		const size_t start = extension.front() == '.' ? 1u : 0u;
		if( start >= extension.size() )
		{
			return {};
		}

		return extension.substr( start );
	}

	std::vector<std::string> normalize_extensions( const std::vector<std::string>& extensions )
	{
		std::vector<std::string> normalized;
		normalized.reserve( extensions.size() );

		for( const std::string& extension : extensions )
		{
			const std::string normalized_extension = normalize_extension( extension );
			if( !normalized_extension.empty() )
			{
				normalized.push_back( normalized_extension );
			}
		}

		return normalized;
	}

	std::string join_extensions( const std::vector<std::string>& extensions )
	{
		std::string spec;
		for( size_t i = 0; i < extensions.size(); ++i )
		{
			if( i > 0 )
			{
				spec += ',';
			}
			spec += extensions[i];
		}
		return spec;
	}

	nfdwindowhandle_t get_native_window_handle( SDL_Window* window )
	{
		nfdwindowhandle_t native_window = {};
		SDL_SysWMinfo wm_info;
		SDL_VERSION( &wm_info.version );
		if( SDL_GetWindowWMInfo( window, &wm_info ) && wm_info.subsystem == SDL_SYSWM_WINDOWS )
		{
			native_window.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
			native_window.handle = wm_info.info.win.window;
		}
		return native_window;
	}
}

FileDialog::FileDialog( SDL_Window* window )
	: mWindow( window )
{
	if( NFD_Init() != NFD_OKAY )
	{
		utility::LOG_ERROR() << "Failed to initialize NFD: " << NFD_GetError();
	}
}

FileDialog::~FileDialog()
{
	NFD_Quit();
}

std::optional<std::string>
FileDialog::GetFilePath( std::vector<std::string> extensions )
{
	const std::vector<std::string> normalized_extensions = normalize_extensions( extensions );
	if( normalized_extensions.empty() )
	{
		utility::LOG_ERROR() << "FileDialog requires at least one valid extension.";
		return std::nullopt;
	}

	const std::string filter_spec = join_extensions( normalized_extensions );
	const std::string filter_name = "Supported files";

	nfdu8filteritem_t filter = {
		filter_name.c_str(),
		filter_spec.c_str(),
	};

	nfdopendialogu8args_t args = {};
	args.filterList = &filter;
	args.filterCount = 1;
	args.parentWindow = get_native_window_handle( mWindow );

	nfdu8char_t* out_path = nullptr;
	const nfdresult_t result = NFD_OpenDialogU8_With( &out_path, &args );
	if( result == NFD_OKAY )
	{
		const std::string selected_path( out_path );
		NFD_FreePathU8( out_path );
		return selected_path;
	}

	if( result == NFD_ERROR )
	{
		utility::LOG_ERROR() << "NFD error: " << NFD_GetError();
	}

	return std::nullopt;
}

}
