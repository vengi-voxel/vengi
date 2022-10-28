/**
 * @file
 */

#include <SDL_platform.h>

#if defined(__LINUX__) || defined(__MACOSX__)
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "io/Filesystem.h"
#include <pwd.h>
#include <unistd.h>

#ifdef __MACOSX__
#include <sysdir.h>
#include <libproc.h>
#endif

namespace io {
namespace priv {

static char *getHome() {
	const int uid = (int)getuid();
	if (uid != 0) {
		return SDL_getenv("HOME");
	}
	return nullptr;
}

/**
 * @brief Replace the shell variable for the home dir
 */
static inline core::String replaceHome(const core::String &in) {
	char *envHome = getHome();
	if (envHome == nullptr) {
		return in;
	}
	core::String out = core::string::replaceAll(in, "$HOME", envHome);
	out = core::string::replaceAll(out, "~", envHome);
	return core::string::replaceAll(out, "${HOME}", envHome);
}

#ifdef __MACOSX__

// needs at least OSX 10.12
static core::String appleDir(sysdir_search_path_directory_t dir) {
	char path[PATH_MAX];
	sysdir_search_path_enumeration_state state = sysdir_start_search_path_enumeration(dir, SYSDIR_DOMAIN_MASK_USER);
	while ((state = sysdir_get_next_search_path_enumeration(state, path))) {
		// indicates a user dir
		if (path[0] == '~') {
			return replaceHome(path);
		}
	}
	return "";
}

#else // __MACOSX__

static core::String load(const core::String &file) {
	FILE *fp = fopen(file.c_str(), "r");
	if (fp == nullptr) {
		Log::debug("Could not open file %s", file.c_str());
		return "";
	}

	if (fseek(fp, 0L, SEEK_END) != 0) {
		Log::debug("Error: fseek failed");
		fclose(fp);
		return "";
	}

	long int bufsize = ftell(fp);
	if (bufsize == -1) {
		Log::debug("Error: ftell failed");
		fclose(fp);
		return "";
	}

	if (fseek(fp, 0L, SEEK_SET) != 0) {
		Log::debug("Error: fseek failed");
		fclose(fp);
		return "";
	}

	char *source = (char *)core_malloc(bufsize + 1);
	size_t newLen = fread(source, 1, bufsize, fp);
	if (ferror(fp) != 0) {
		perror("Error: failed to read shader file. ");
		fclose(fp);
		return "";
	}
	fclose(fp);

	source[newLen++] = '\0';
	core::String str(source);
	core_free(source);
	return str;
}

#endif

} // namespace priv

bool initState(io::FilesystemState &state) {
	char *envHome = priv::getHome();
	if (envHome == nullptr) {
		Log::debug("Can't read xdg user dirs: HOME env var not found");
		return false;
	}
#if defined __MACOSX__
	state._directories[FilesystemDirectories::FS_Dir_Download] = priv::appleDir(SYSDIR_DIRECTORY_DOWNLOADS);
	state._directories[FilesystemDirectories::FS_Dir_Documents] = priv::appleDir(SYSDIR_DIRECTORY_DOCUMENT);
	state._directories[FilesystemDirectories::FS_Dir_Pictures] = priv::appleDir(SYSDIR_DIRECTORY_PICTURES);
	state._directories[FilesystemDirectories::FS_Dir_Desktop] = priv::appleDir(SYSDIR_DIRECTORY_DESKTOP);
	state._directories[FilesystemDirectories::FS_Dir_Public] = priv::appleDir(SYSDIR_DIRECTORY_SHARED_PUBLIC);
#else
	core::String xdgDir = core::string::path(envHome, ".config", "user-dirs.dirs");
	if (access(xdgDir.c_str(), F_OK) != 0) {
		Log::debug("Can't read xdg user dirs: %s doesn't exists", xdgDir.c_str());
		const char *xdgConfigDirs = SDL_getenv("XDG_CONFIG_DIRS");
		if (xdgConfigDirs == nullptr) {
			xdgConfigDirs = "/etc/xdg";
		}
		xdgDir = core::string::path(xdgConfigDirs, "user-dirs.defaults");
		if (access(xdgDir.c_str(), F_OK) != 0) {
			Log::debug("Can't read xdg dirs: %s doesn't exists", xdgDir.c_str());
			return false;
		}
	}
	const core::String &xdgDirsContent = priv::load(xdgDir);
	if (xdgDirsContent.empty()) {
		Log::debug("Could not read %s", xdgDir.c_str());
		return false;
	}
	core::TokenizerConfig cfg;
	core::Tokenizer tok(cfg, xdgDirsContent, "=");
	while (tok.hasNext()) {
		const core::String var = tok.next();
		if (!tok.hasNext()) {
			return false;
		}
		// https://www.freedesktop.org/wiki/Software/xdg-user-dirs/
		const core::String value = tok.next();
		if (var.contains("DOWNLOAD")) {
			state._directories[FilesystemDirectories::FS_Dir_Download] = priv::replaceHome(value);
		} else if (var.contains("DOCUMENTS")) {
			state._directories[FilesystemDirectories::FS_Dir_Documents] = priv::replaceHome(value);
		} else if (var.contains("DESKTOP")) {
			state._directories[FilesystemDirectories::FS_Dir_Desktop] = priv::replaceHome(value);
		} else if (var.contains("PICTURES")) {
			state._directories[FilesystemDirectories::FS_Dir_Pictures] = priv::replaceHome(value);
		} else if (var.contains("PUBLICSHARE")) {
			state._directories[FilesystemDirectories::FS_Dir_Public] = priv::replaceHome(value);
		}
	}
#endif

	for (int n = 0; n < FilesystemDirectories::FS_Dir_Max; ++n) {
		if (state._directories[n].empty()) {
			continue;
		}
		if (core::string::isAbsolutePath(state._directories[n])) {
			continue;
		}
		state._directories[n] = core::string::path(envHome, state._directories[n]);
	}

	return true;
}

} // namespace io

#endif
