#ifndef _ANIMTOOL_FILE_DIALOG_H_
#define _ANIMTOOL_FILE_DIALOG_H_

#include <optional>
#include <string>
#include <vector>

struct SDL_Window;

namespace animtool
{

class FileDialog
{
public:
	explicit FileDialog( SDL_Window* window );
	~FileDialog();

	std::optional<std::string> GetFilePath( std::vector<std::string> extensions );
	std::optional<std::string> GetFolderPath();

	FileDialog( const FileDialog& ) = delete;
	FileDialog& operator=( const FileDialog& ) = delete;

private:
	SDL_Window* mWindow;
};

}

#endif // _ANIMTOOL_FILE_DIALOG_H_
