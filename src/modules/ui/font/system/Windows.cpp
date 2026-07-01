/**
 * @file
 */

#include "ui/font/FontResolver.h"

#if defined(_WIN32) || defined(__CYGWIN__)

#include "core/Log.h"
#include "core/StringUtil.h"
#include <SDL3/SDL_stdinc.h>
#include <shlobj.h>
#include <windows.h>

namespace ui {
namespace font {

core::DynamicArray<core::String> findSystemFonts() {
	core::DynamicArray<core::String> results;

	WCHAR fontsPath[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, fontsPath))) {
		return results;
	}

	char *utf8Dir = SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)fontsPath,
									 (SDL_wcslen(fontsPath) + 1) * sizeof(WCHAR));
	if (utf8Dir == nullptr) {
		return results;
	}
	const core::String dir(utf8Dir);
	SDL_free(utf8Dir);

	const core::String searchPath = dir + "\\*";
	WCHAR *wpath = (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", searchPath.c_str(), searchPath.size() + 1);
	if (wpath == nullptr) {
		return results;
	}

	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(wpath, &fd);
	SDL_free(wpath);
	if (hFind == INVALID_HANDLE_VALUE) {
		return results;
	}

	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		char *utf8Name = SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)fd.cFileName,
										  (SDL_wcslen(fd.cFileName) + 1) * sizeof(WCHAR));
		if (utf8Name == nullptr) {
			continue;
		}
		const core::String name(utf8Name);
		SDL_free(utf8Name);
		if (core::string::endsWith(name, ".ttf", true) || core::string::endsWith(name, ".ttc", true) ||
			core::string::endsWith(name, ".otf", true)) {
			results.push_back(dir + "\\" + name);
		}
	} while (FindNextFileW(hFind, &fd));

	FindClose(hFind);

	Log::debug("Found %d system fonts", (int)results.size());
	return results;
}

} // namespace font
} // namespace ui

#endif // _WIN32 || __CYGWIN__
