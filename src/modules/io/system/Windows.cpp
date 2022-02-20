/**
 * @file
 */

#include <SDL_platform.h>

#if defined(__WINDOWS__)
#include "core/String.h"
#include "core/Log.h"
#include "io/Filesystem.h"
#include <SDL_stdinc.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <initguid.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <wchar.h>
#include <windows.h>

namespace io {
namespace priv {

static core::String knownFolderPath(REFKNOWNFOLDERID id) {
	PWSTR path = NULL;
	HRESULT hr = SHGetKnownFolderPath(id, 0, NULL, &path);
	char *retval;

	if (!SUCCEEDED(hr)) {
		Log::debug("Failed to get a known folder path");
		return "";
	}

	retval = SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)(path), (SDL_wcslen(path) + 1) * sizeof(WCHAR));
	CoTaskMemFree(path);
	const core::String strpath(retval);
	SDL_free(retval);
	return strpath;
}

} // namespace priv

bool initState(io::FilesystemState &state) {
	// https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid
	state._directories[FilesystemDirectories::FS_Dir_Documents] = priv::knownFolderPath(FOLDERID_Documents);
	state._directories[FilesystemDirectories::FS_Dir_Download] = priv::knownFolderPath(FOLDERID_Downloads);
	state._directories[FilesystemDirectories::FS_Dir_Pictures] = priv::knownFolderPath(FOLDERID_Pictures);
	state._directories[FilesystemDirectories::FS_Dir_Public] = priv::knownFolderPath(FOLDERID_Public);
	state._directories[FilesystemDirectories::FS_Dir_Recent] = priv::knownFolderPath(FOLDERID_Recent);
	state._directories[FilesystemDirectories::FS_Dir_Cloud] = priv::knownFolderPath(FOLDERID_SkyDrive);
	return true;
}

} // namespace io

#endif