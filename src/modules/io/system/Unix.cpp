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
#include <unistd.h>

namespace io {
namespace priv {

/**
 * @brief Replace the shell variable for the home dir
 */
static inline core::String replaceHome(const core::String &in) {
	char *envHome = SDL_getenv("HOME");
	if (envHome == nullptr) {
		return in;
	}
	core::String out = core::string::replaceAll(in, "$HOME", envHome);
	return core::string::replaceAll(out, "${HOME}", envHome);
}

static core::String load(const core::String &file) {
	FILE *fp = fopen(file.c_str(), "r");
	if (fp == NULL) {
		Log::debug("Could not open file %s", file.c_str());
		return "";
	}

	if (fseek(fp, 0L, SEEK_END) != 0) {
		Log::debug("Error: fseek failed");
		return "";
	}

	long int bufsize = ftell(fp);
	if (bufsize == -1) {
		Log::debug("Error: ftell failed");
		return "";
	}

	if (fseek(fp, 0L, SEEK_SET) != 0) {
		Log::debug("Error: fseek failed");
		return "";
	}

	char *source = (char *)core_malloc(bufsize + 1);
	size_t newLen = fread(source, 1, bufsize, fp);
	if (ferror(fp) != 0) {
		perror("Error: failed to read shader file. ");
		return "";
	}

	source[newLen++] = '\0';
	core::String str(source);
	core_free(source);
	return str;
}

} // namespace priv

bool initState(io::FilesystemState &state) {
	char *envHome = SDL_getenv("HOME");
	if (envHome == nullptr) {
		Log::debug("Can't read xdg user dirs: HOME env var not found");
		return false;
	}
	const core::String &xdgUserDirs = core::string::path(envHome, ".config", "user-dirs.dirs");
	if (access(xdgUserDirs.c_str(), F_OK) != 0) {
		Log::debug("Can't read xdg user dirs: %s doesn't exists", xdgUserDirs.c_str());
		return false;
	}
	const core::String &xdgUserDirsContent = priv::load(xdgUserDirs);
	if (xdgUserDirsContent.empty()) {
		Log::debug("Could not read %s", xdgUserDirs.c_str());
		return false;
	}
	core::Tokenizer tok(true, xdgUserDirsContent, "=");
	while (tok.hasNext()) {
		const core::String var = tok.next();
		if (!tok.hasNext()) {
			return false;
		}
		const core::String value = tok.next();
		if (var == "XDG_DOWNLOAD_DIR") {
			state._downloadDir = priv::replaceHome(value);
		} else if (var == "XDG_DOCUMENTS_DIR") {
			state._documentsDir = priv::replaceHome(value);
		}
	}

	return true;
}

} // namespace io

#endif
