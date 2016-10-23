#include "Filesystem.h"
#include <windows.h>
#include <direct.h>
#include <wchar.h>

namespace io {

bool Filesystem::createDir(const std::string& dir) const {
	WCHAR wpath[1024];
	const int len = MultiByteToWideChar(CP_UTF8, 0, dir.c_str(), -1, wpath, SDL_arraysize(wpath) - 1);
	wpath[len] = 0;
	_wmkdir(wpath);
	return true;
}

bool Filesystem::list(const std::string& directory, std::vector<DirEntry>& entities, const std::string& filter) const {
	return false;
}

}
