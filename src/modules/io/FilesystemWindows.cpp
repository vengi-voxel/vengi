#include "Filesystem.h"
#include "core/String.h"
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <wchar.h>

namespace io {

bool Filesystem::chdir(const std::string& directory) {
	// TODO: implement me
	return false;
}

bool Filesystem::createDir(const std::string& directory) const {
	if (directory.empty()) {
		return false;
	}
	const bool bCD = CreateDirectory(directory.c_str(), nullptr);
	if (bCD) {
		return true;
	}

	DWORD dwLastError = GetLastError();
	switch (dwLastError) {
	case ERROR_ALREADY_EXISTS:
		return true;
	case ERROR_PATH_NOT_FOUND: {
		std::size_t sep = directory.rfind('/');
		if (sep == std::string::npos) {
			sep = directory.rfind('\\');
		}
		if (sep == std::string::npos) {
			return false;
		}
		const std::string& szPrev = directory.substr(0, sep);
		if (CreateDirectory(szPrev.c_str(), nullptr)) {
			const bool bSuccess = CreateDirectory(directory.c_str(), nullptr) != 0;
			if (bSuccess) {
				return true;
			}
			return GetLastError() == ERROR_ALREADY_EXISTS;
		}
		return false;
	}
	default:
		return false;
	}
}

bool Filesystem::list(const std::string& directory, std::vector<DirEntry>& entities, const std::string& filter) const {
	std::string search(directory + "\\*");

	struct _finddata_t findinfo;
	const int dir = _findfirst(search.c_str(), &findinfo);
	if (dir == -1) {
		return false;
	}

	do {
		DirEntry::Type type = DirEntry::Type::unknown;
		if (findinfo.attrib & _A_SUBDIR) {
			if (!strcmp(findinfo.name, ".") || !strcmp(findinfo.name, "..")) {
				continue;
			}
			type = DirEntry::Type::dir;
		} else {
			type = DirEntry::Type::file;
		}
		if (!core::string::matches(filter, findinfo.name)) {
			continue;
		}
		entities.push_back(DirEntry{findinfo.name, type});
	} while (_findnext(dir, &findinfo) != -1);

	_findclose(dir);
	return true;
}

}
