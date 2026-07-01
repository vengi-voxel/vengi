/**
 * @file
 */

#include "ui/font/FontResolver.h"

#if (defined(__linux__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)

#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include <SDL3/SDL_stdinc.h>
#include <dirent.h>
#include <sys/stat.h>

namespace ui {
namespace font {

static void scanDir(const char *dir, core::DynamicArray<core::String> &results, bool recurse = true) {
	DIR *d = opendir(dir);
	if (d == nullptr) {
		return;
	}
	struct dirent *entry;
	while ((entry = readdir(d)) != nullptr) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		const core::String fullPath = core::string::path(dir, entry->d_name);
		struct stat st;
		if (stat(fullPath.c_str(), &st) != 0) {
			continue;
		}
		if (S_ISDIR(st.st_mode) && recurse) {
			scanDir(fullPath.c_str(), results, true);
		} else if (S_ISREG(st.st_mode)) {
			const core::String name(entry->d_name);
			if (core::string::endsWith(name, ".ttf", true) || core::string::endsWith(name, ".ttc", true) ||
				core::string::endsWith(name, ".otf", true)) {
				results.push_back(fullPath);
			}
		}
	}
	closedir(d);
}

core::DynamicArray<core::String> findSystemFonts() {
	core::DynamicArray<core::String> results;

#ifdef __APPLE__
	static const char *fontDirs[] = {
		"/System/Library/Fonts",
		"/Library/Fonts",
	};
#else
	static const char *fontDirs[] = {
		"/usr/share/fonts",
		"/usr/local/share/fonts",
	};
#endif

	for (int i = 0; i < lengthof(fontDirs); ++i) {
		scanDir(fontDirs[i], results);
	}

#ifdef __APPLE__
	const char *home = SDL_getenv("HOME");
	if (home != nullptr) {
		const core::String userFonts = core::String::format("%s/Library/Fonts", home);
		scanDir(userFonts.c_str(), results);
	}
#else
	const char *home = SDL_getenv("HOME");
	if (home != nullptr) {
		// $XDG_DATA_HOME/fonts
		// fontconfig?
		const core::String userFonts = core::String::format("%s/.local/share/fonts", home);
		scanDir(userFonts.c_str(), results);
	}
#endif

	Log::debug("Found %d system fonts", (int)results.size());
	return results;
}

} // namespace font
} // namespace ui

#endif // (__linux__ || __APPLE__) && !__EMSCRIPTEN__
