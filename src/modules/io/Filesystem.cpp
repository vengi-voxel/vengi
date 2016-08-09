/**
 * @file
 */

#include "Filesystem.h"
#include "core/Var.h"
#include "core/Log.h"
#include <SDL.h>

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

	Log::debug("basepath: %s", _basePath.c_str());
	Log::debug("homepath: %s", _homePath.c_str());
	core::Var::get(cfg::AppHomePath, _homePath, core::CV_READONLY | core::CV_NOPERSIST);
	core::Var::get(cfg::AppBasePath, _basePath, core::CV_READONLY | core::CV_NOPERSIST);
}

io::FilePtr Filesystem::open(const std::string& filename) {
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

std::string Filesystem::load(const std::string& filename) {
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
	// TODO: windows
	return ::mkdir(path.c_str(), 0700);
}

}
