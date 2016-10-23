/**
 * @file
 */

#include "Filesystem.h"
#include "core/Var.h"
#include "core/Log.h"
#include <SDL.h>
#if __WINDOWS__
#include <windows.h>
#include <direct.h>
#include <wchar.h>
#elif __LINUX__ or __MACOSX__
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if __cplusplus <= 201411
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::std::filesystem;
#endif

namespace io {

Filesystem::Filesystem() :
		_threadPool(1, "IO") {
}

void Filesystem::init(const std::string& organisation, const std::string& appname) {
	_organisation = organisation;
	_appname = appname;

	char *path = SDL_GetBasePath();
	if (path == nullptr) {
		_basePath = "";
	} else {
		_basePath = path;
		SDL_free(path);
	}

	char *prefPath = SDL_GetPrefPath(_organisation.c_str(), _appname.c_str());
	if (prefPath == nullptr) {
		_homePath = "";
	} else {
		_homePath = prefPath;
		SDL_free(prefPath);
	}
	std::error_code errorCode;
	const fs::space_info& s = fs::space(fs::path(_homePath), errorCode);
	constexpr uintmax_t div = 1024 * 1024;
	const uint32_t capacity = s.capacity / div;
	const uint32_t free = s.free / div;
	const uint32_t available = s.available / div;

	Log::debug("basepath: %s", _basePath.c_str());
	Log::debug("homepath: %s (capacity: %i MB, free: %i MB, available: %i MB)", _homePath.c_str(), capacity, free, available);
	core::Var::get(cfg::AppHomePath, _homePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);
	core::Var::get(cfg::AppBasePath, _basePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);
}

bool Filesystem::list(const std::string& directory, std::vector<DirEntry>& entries, const std::string& filter) const {
	const fs::path path(directory);
	std::error_code errorCode;
	for (const fs::directory_entry& p: fs::directory_iterator(path, errorCode)) {
		const std::string& s = p.path().string();
		if (!filter.empty() && !core::string::matches(filter, s)) {
			continue;
		}
		const fs::file_type fileType = p.status().type();
		DirEntry::Type type = DirEntry::Type::unknown;
		switch (fileType) {
		case fs::file_type::regular:
			type = DirEntry::Type::file;
			break;
		case fs::file_type::directory:
			type = DirEntry::Type::dir;
			break;
		case fs::file_type::symlink:
			type = DirEntry::Type::symlink;
			break;
		case fs::file_type::fifo:
			type = DirEntry::Type::fifo;
			break;
		case fs::file_type::socket:
			type = DirEntry::Type::socket;
			break;
		default:
			break;
		}
		const DirEntry d{s, type};
		entries.push_back(d);
	}
	return (bool)errorCode;
}

io::FilePtr Filesystem::open(const std::string& filename) const {
	if (io::File(filename).exists()) {
		Log::debug("loading file %s from current working dir", filename.c_str());
		return std::make_shared<io::File>(filename);
	}
	const std::string homePath = _homePath + filename;
	if (io::File(homePath).exists()) {
		Log::debug("loading file %s from %s", filename.c_str(), _homePath.c_str());
		return std::make_shared<io::File>(homePath);
	}
	Log::debug("loading file %s from %s (doesn't exist at %s)", filename.c_str(), _basePath.c_str(), homePath.c_str());
	return std::make_shared<io::File>(_basePath + filename);
}

std::string Filesystem::load(const std::string& filename) const {
	const io::FilePtr& f = open(filename);
	return f->load();
}

bool Filesystem::write(const std::string& filename, const uint8_t* content, size_t length) {
	io::File f(_homePath + filename);
	createDir(f.getPath());
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::write(const std::string& filename, const std::string& string) {
	const uint8_t* buf = reinterpret_cast<const uint8_t*>(string.c_str());
	return write(filename, buf, string.size());
}

bool Filesystem::syswrite(const std::string& filename, const uint8_t* content, size_t length) {
	io::File f(filename);
	createDir(f.getPath());
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::syswrite(const std::string& filename, const std::string& string) {
	const uint8_t* buf = reinterpret_cast<const uint8_t*>(string.c_str());
	return syswrite(filename, buf, string.size());
}

bool Filesystem::createDir(const std::string& path) const {
	std::error_code errorCode;
	return fs::create_directories(fs::path(path), errorCode);
}

}
