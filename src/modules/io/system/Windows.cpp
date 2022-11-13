/**
 * @file
 */

#include <SDL_platform.h>

#if defined(__WINDOWS__)
#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "core/ArrayLength.h"
#include "core/String.h"
#include "core/Log.h"
#include "io/Filesystem.h"
#include <SDL_stdinc.h>

#include "dirent.h"
#include <initguid.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <wchar.h>
#include <sys/stat.h>

namespace io {
namespace priv {

#define io_StringToUTF8W(S) SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define io_UTF8ToStringW(S) (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", (const char *)(S), SDL_strlen(S)+1)

static core::String knownFolderPath(REFKNOWNFOLDERID id) {
	PWSTR path = NULL;
	HRESULT hr = SHGetKnownFolderPath(id, 0, NULL, &path);
	char *retval;

	if (!SUCCEEDED(hr)) {
		Log::debug("Failed to get a known folder path");
		return "";
	}

	retval = io_StringToUTF8W(path);
	CoTaskMemFree(path);
	const core::String strpath(retval);
	SDL_free(retval);
	return strpath;
}

} // namespace priv

bool initState(io::FilesystemState &state) {
	// https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid
	state._directories[FilesystemDirectories::FS_Dir_Documents] = priv::knownFolderPath(FOLDERID_Documents);
	state._directories[FilesystemDirectories::FS_Dir_Desktop] = priv::knownFolderPath(FOLDERID_Desktop);
	state._directories[FilesystemDirectories::FS_Dir_Download] = priv::knownFolderPath(FOLDERID_Downloads);
	state._directories[FilesystemDirectories::FS_Dir_Pictures] = priv::knownFolderPath(FOLDERID_Pictures);
	state._directories[FilesystemDirectories::FS_Dir_Public] = priv::knownFolderPath(FOLDERID_Public);
	state._directories[FilesystemDirectories::FS_Dir_Recent] = priv::knownFolderPath(FOLDERID_Recent);
	state._directories[FilesystemDirectories::FS_Dir_Cloud] = priv::knownFolderPath(FOLDERID_SkyDrive);
	return true;
}

bool fs_mkdir(const char *path) {
	WCHAR *wpath = io_UTF8ToStringW(path);
	const int ret = _wmkdir(wpath);
	SDL_free(wpath);
	if (ret != 0 && GetLastError() == ERROR_ALREADY_EXISTS) {
		return true;
	}
	return ret == 0;
}

bool fs_remove(const char *path) {
	WCHAR *wpath = io_UTF8ToStringW(path);
	const int ret = _wremove(wpath);
	SDL_free(wpath);
	return ret == 0;
}

bool fs_exists(const char *path) {
	WCHAR *wpath = io_UTF8ToStringW(path);
	const int ret = _waccess(wpath, 00);
	SDL_free(wpath);
	return ret == 0;
}

bool fs_chdir(const char *path) {
	WCHAR *wpath = io_UTF8ToStringW(path);
	const bool ret = SetCurrentDirectoryW(wpath);
	SDL_free(wpath);
	return ret;
}

core::String fs_realpath(const char *path) {
	WCHAR *wpath = io_UTF8ToStringW(path);
	WCHAR wfull[_MAX_PATH];
	if (_wfullpath(wfull, wpath, lengthof(wfull)) == nullptr) {
		SDL_free(wpath);
		return "";
	}
	SDL_free(wpath);
	char *full = io_StringToUTF8W(wfull);
	const core::String str(full);
	free(full);
	return str;
}

bool fs_stat(const char *path, FilesystemEntry &entry) {
	struct _stat s;
	const int result = _stat(path, &s);
	if (result == 0) {
		if (entry.type == FilesystemEntry::Type::unknown) {
			entry.type = (s.st_mode & _S_IFDIR) ? FilesystemEntry::Type::dir : FilesystemEntry::Type::file;
		}
		entry.mtime = (uint64_t)s.st_mtime * 1000;
		entry.size = s.st_size;
		return true;
	}
	return false;
}

core::String fs_readlink(const char *path) {
	return "";
}

static int fs_scandir_filter(const struct dirent *dent) {
	return strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0;
}

core::DynamicArray<FilesystemEntry> fs_scandir(const char *path) {
	struct dirent **files = nullptr;
	const int n = scandir(path, &files, fs_scandir_filter, alphasort);
	core::DynamicArray<FilesystemEntry> entries;
	entries.reserve(n);
	for (int i = 0; i < n; ++i) {
		const struct dirent *ent = files[i];
		FilesystemEntry entry;
		entry.name = ent->d_name;
		switch (ent->d_type) {
		case DT_DIR:
			entry.type = FilesystemEntry::Type::dir;
			break;
		case DT_REG:
			entry.type = FilesystemEntry::Type::file;
			break;
		case DT_LNK:
			entry.type = FilesystemEntry::Type::link;
			break;
		default:
			entry.type = FilesystemEntry::Type::unknown;
			break;
		}
		entries.push_back(entry);
	}
	for (int i = 0; i < n; i++) {
		free(files[i]);
	}
	free(files);
	return entries;
}

#undef io_StringToUTF8W
#undef io_UTF8ToStringW

} // namespace io

#endif
