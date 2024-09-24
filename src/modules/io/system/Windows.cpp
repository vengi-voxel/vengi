/**
 * @file
 */

#include <SDL_platform.h>

#if defined(__WINDOWS__)
#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "core/ArrayLength.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "io/Filesystem.h"
#include <SDL_stdinc.h>

#include "windirent.h"
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
	return core::string::sanitizeDirPath(strpath);
}

static void denormalizePath(WCHAR* str) {
	WCHAR *p = str;
	while (*p != 0) {
		if (*p == L'/') {
			*p = L'\\';
		}
		++p;
	}
}

} // namespace priv

bool initState(io::FilesystemState &state) {
	// https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid
	state._directories[FilesystemDirectories::FS_Dir_Documents] = priv::knownFolderPath(FOLDERID_Documents);
	state._directories[FilesystemDirectories::FS_Dir_Desktop] = priv::knownFolderPath(FOLDERID_Desktop);
	state._directories[FilesystemDirectories::FS_Dir_Download] = priv::knownFolderPath(FOLDERID_Downloads);
	state._directories[FilesystemDirectories::FS_Dir_Pictures] = priv::knownFolderPath(FOLDERID_Pictures);
	state._directories[FilesystemDirectories::FS_Dir_Public] = priv::knownFolderPath(FOLDERID_Public);
	state._directories[FilesystemDirectories::FS_Dir_Fonts] = priv::knownFolderPath(FOLDERID_Fonts);
	state._directories[FilesystemDirectories::FS_Dir_Recent] = priv::knownFolderPath(FOLDERID_Recent);
	state._directories[FilesystemDirectories::FS_Dir_Cloud] = priv::knownFolderPath(FOLDERID_SkyDrive);

	const DWORD drives = GetLogicalDrives();
	for (int i = 0; i < 26; ++i) {
		if ((drives & (1 << i)) == 0) {
			continue;
		}
		const char driveStr[4] = {(char)('A' + i), ':', '/', '\0'};
		const core::String description = core::string::format("Drive %c", 'A' + i);
		state._thisPc.push_back({description, driveStr});
	}

	return true;
}

bool fs_mkdir(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const int ret = _wmkdir(wpath);
	SDL_free(wpath);
	if (ret == 0) {
		return true;
	}
	if (errno == EEXIST) {
		return true;
	}
	Log::error("Failed to mkdir %s: %s", path.c_str(), strerror(errno));
	return false;
}

bool fs_unlink(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const int ret = _wunlink(wpath);
	SDL_free(wpath);
	if (ret != 0) {
		Log::error("Failed to unlink %s: %s", path.c_str(), strerror(errno));
	}
	return ret == 0;
}

bool fs_rmdir(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const int ret = _wrmdir(wpath);
	SDL_free(wpath);
	if (ret != 0) {
		Log::error("Failed to rmdir %s: %s", path.c_str(), strerror(errno));
	}
	return ret == 0;
}

bool fs_hidden(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);

	DWORD attributes = GetFileAttributesW(wpath);
	SDL_free(wpath);

	if (attributes == INVALID_FILE_ATTRIBUTES) {
		Log::debug("Failed to get file attributes for %s: %s", path.c_str(), strerror(errno));
		return false;
	}

	return (attributes & FILE_ATTRIBUTE_HIDDEN) != 0;
}

bool fs_exists(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const int ret = _waccess(wpath, 0);
	SDL_free(wpath);
	if (ret != 0) {
		Log::trace("Failed to access %s: %s", path.c_str(), strerror(errno));
	}
	return ret == 0;
}

bool fs_writeable(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const int ret = _waccess(wpath, 2);
	SDL_free(wpath);
	return ret == 0;
}

bool fs_chdir(const core::Path& path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const bool ret = SetCurrentDirectoryW(wpath);
	SDL_free(wpath);
	if (!ret) {
		Log::error("Failed to chdir to %s: %s", path.c_str(), strerror(errno));
	}
	return ret;
}

core::Path fs_cwd() {
	WCHAR buf[4096];
	const WCHAR *p = _wgetcwd(buf, lengthof(buf));
	if (p == nullptr) {
		Log::error("Failed to get current working dir: %s", strerror(errno));
		return core::Path();
	}
	char *utf8 = io_StringToUTF8W(p);
	const core::String str(utf8);
	SDL_free(utf8);
	return core::Path(str);
}

core::Path fs_realpath(const core::Path &path) {
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	WCHAR wfull[_MAX_PATH];
	if (_wfullpath(wfull, wpath, lengthof(wfull)) == nullptr) {
		SDL_free(wpath);
		return core::Path();
	}
	SDL_free(wpath);
	priv::denormalizePath(wfull);
	char *full = io_StringToUTF8W(wfull);
	const core::String str(full);
	SDL_free(full);
	return core::Path(str);
}

bool fs_stat(const core::Path &path, FilesystemEntry &entry) {
	struct _stat s;
	WCHAR *wpath = io_UTF8ToStringW(path.c_str());
	priv::denormalizePath(wpath);
	const int ret = _wstat(wpath, &s);
	SDL_free(wpath);
	if (ret == 0) {
		if (entry.type == FilesystemEntry::Type::unknown) {
			entry.type = (s.st_mode & _S_IFDIR) ? FilesystemEntry::Type::dir : FilesystemEntry::Type::file;
		}
		entry.mtime = (uint64_t)s.st_mtime * 1000;
		entry.size = s.st_size;
		return true;
	}
	Log::debug("Failed to stat %s: %s", path.c_str(), strerror(errno));
	return false;
}

core::Path fs_readlink(const core::Path &path) {
	return path;
}

static int fs_scandir_filter(const struct dirent *dent) {
	return strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0;
}

core::DynamicArray<FilesystemEntry> fs_scandir(const core::Path &path) {
	struct dirent **files = nullptr;
	const int n = scandir(path.c_str(), &files, fs_scandir_filter, alphasort);
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
