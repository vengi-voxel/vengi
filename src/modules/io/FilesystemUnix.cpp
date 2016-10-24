#include "Filesystem.h"
#include "core/String.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>

namespace io {

bool Filesystem::createDir(const std::string& dir) const {
	if (dir.empty()) {
		return false;
	}
	std::string s = dir;
	if (s[s.size() - 1] != '/') {
		// force trailing / so we can handle everything in loop
		s += '/';
	}

	size_t pre = 0, pos;
	while ((pos = s.find_first_of('/', pre)) != std::string::npos) {
		const std::string dir = s.substr(0, pos++);
		pre = pos;
		if (dir.empty()) {
			continue; // if leading / first time is 0 length
		}
		const char *dirc = dir.c_str();
		const int retVal = ::mkdir(dirc, 0700);
		if (retVal == -1 && errno != EEXIST) {
			return false;
		}
	}
	return true;
}

bool Filesystem::list(const std::string& directory, std::vector<DirEntry>& entities, const std::string& filter) const {
	DIR *dir = ::opendir(directory.c_str());
	if (dir == nullptr) {
		return false;
	}

	std::string search(directory + "/");
	struct dirent *d;
	while ((d = ::readdir(dir)) != nullptr) {
		std::string name(search);
		name.append(d->d_name);
		struct stat st;
		if (::stat(name.c_str(), &st) == -1) {
			continue;
		}
		DirEntry::Type type = DirEntry::Type::unknown;
		if (st.st_mode & S_IFDIR) {
			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) {
				continue;
			}
			type = DirEntry::Type::dir;
		} else {
			type = DirEntry::Type::file;
		}
		if (!core::string::matches(filter, d->d_name)) {
			continue;
		}
		entities.push_back(DirEntry{d->d_name, type});
	}

	::closedir(dir);

	return true;
}

}
