/**
 * @file
 */

#include "Filesystem.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include <SDL.h>

namespace io {

MAKE_SHARED_INVIS_CTOR(File);

Filesystem::~Filesystem() {
	shutdown();
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
		createDir(_homePath);
	}

	registerPath(_basePath);
	registerPath(_homePath);

	Log::debug("basepath: %s", _basePath.c_str());
	Log::debug("homepath: %s", _homePath.c_str());
	core::Var::get(cfg::AppHomePath, _homePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);
	core::Var::get(cfg::AppBasePath, _basePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);

	_loop = new uv_loop_t;
	uv_loop_init(_loop);
}

bool Filesystem::removeDir(const std::string& dir, bool recursive) const {
	if (dir.empty()) {
		return false;
	}

	if (!recursive) {
		uv_fs_t req;
		return uv_fs_rmdir(_loop, &req, dir.c_str(), nullptr) == 0;
	}
	// TODO: implement me
	return false;
}

bool Filesystem::createDir(const std::string& dir, bool recursive) const {
	if (dir.empty()) {
		return false;
	}

	if (!recursive) {
		uv_fs_t req;
		// rwx+o, r+g
		const int retVal = uv_fs_mkdir(_loop, &req, dir.c_str(), 0740, nullptr);
		if (retVal != 0 && req.result != UV_EEXIST) {
			return false;
		}
		return true;
	}

	std::string s = dir;
	if (s[s.size() - 1] != '/') {
		// force trailing / so we can handle everything in loop
		s += '/';
	}

	size_t pre = 0, pos;
	while ((pos = s.find_first_of('/', pre)) != std::string::npos) {
		const std::string dirpart = s.substr(0, pos++);
		pre = pos;
		if (dirpart.empty()) {
			continue; // if leading / first time is 0 length
		}
		const char *dirc = dirpart.c_str();
		uv_fs_t req;
		const int retVal = uv_fs_mkdir(_loop, &req, dirc, 0700, nullptr);
		if (retVal != 0 && req.result != UV_EEXIST) {
			return false;
		}
	}
	return true;
}

bool Filesystem::list(const std::string& directory, std::vector<DirEntry>& entities, const std::string& filter) const {
	uv_fs_t req;
	const int amount = uv_fs_scandir(_loop, &req, directory.c_str(), 0, nullptr);
	if (amount <= 0) {
		return false;
	}
	uv_dirent_t ent;
	while (uv_fs_scandir_next(&req, &ent) != UV_EOF) {
		DirEntry::Type type = DirEntry::Type::unknown;
		if (ent.type == UV_DIRENT_DIR) {
			type = DirEntry::Type::dir;
		} else if (ent.type == UV_DIRENT_FILE) {
			type = DirEntry::Type::file;
		} else if (ent.type == UV_DIRENT_UNKNOWN) {
			type = DirEntry::Type::unknown;
		} else {
			Log::debug("Unknown directory entry found: %s", ent.name);
			continue;
		}
		if (!filter.empty() && !core::string::matches(filter, ent.name)) {
			continue;
		}
		entities.push_back(DirEntry{ent.name, type});
	}
	return true;
}

void Filesystem::update() {
	uv_run(_loop, UV_RUN_NOWAIT);
}

bool Filesystem::chdir(const std::string& directory) {
	return uv_chdir(directory.c_str()) == 0;
}

void Filesystem::shutdown() {
	for (auto& e : _watches) {
		uv_fs_event_stop(e.second);
	}
	if (_loop != nullptr) {
		uv_loop_close(_loop);
		delete _loop;
		_loop = nullptr;
	}
	for (auto& e : _watches) {
		delete e.second;
	}
	_watches.clear();
}

bool Filesystem::isRelativeFilename(const std::string& name) const {
	const size_t size = name.size();
#ifdef __WINDOWS__
	if (size < 3) {
		return true;
	}
	// TODO: hm... not cool and most likely not enough
	return name[1] != ':';
#else
	if (size == 0) {
		return false;
	}
	return name[0] != '/';
#endif
}

bool Filesystem::registerPath(const std::string& path) {
	if (!core::string::endsWith(path, "/")) {
		return false;
	}
	_paths.push_back(path);
	return true;
}

bool Filesystem::unwatch(const std::string& path) {
	auto i = _watches.find(path);
	if (i == _watches.end()) {
		return false;
	}
	uv_fs_event_stop(i->second);
	delete i->second;
	_watches.erase(i);
	return true;
}

bool Filesystem::watch(const std::string& path, FileWatcher watcher) {
	uv_fs_event_t* fsEvent = new uv_fs_event_t;
	if (uv_fs_event_init(_loop, fsEvent) != 0) {
		delete fsEvent;
		return false;
	}
	fsEvent->data = (void*)watcher;
	auto i = _watches.insert(std::make_pair(path, fsEvent));
	if (!i.second) {
		delete fsEvent;
		return false;
	}
	const int ret = uv_fs_event_start(fsEvent, [] (uv_fs_event_t *handle, const char *filename, int events, int status) {
		if ((events & UV_CHANGE) == 0) {
			return;
		}
		if (filename == nullptr) {
			return;
		}
		FileWatcher watcher = (FileWatcher)handle->data;
		watcher(filename);
	}, path.c_str(), 0);
	if (ret != 0) {
		_watches.erase(_watches.find(path));
		delete fsEvent;
		return false;
	}
	return true;
}

bool Filesystem::popDir() {
	if (_dirStack.empty()) {
		return false;
	}
	const std::string& directory = _dirStack.top();
	chdir(directory);
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.pop();
	return true;
}

bool Filesystem::pushDir(const std::string& directory) {
	const bool changed = chdir(directory);
	if (!changed) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

io::FilePtr Filesystem::open(const std::string& filename, FileMode mode) const {
	if (mode == FileMode::Write && !isRelativeFilename(filename)) {
		return std::make_shared<make_shared_enabler>(filename, mode);
	}
	if (io::File(filename, FileMode::Read).exists()) {
		Log::debug("loading file %s from current working dir", filename.c_str());
		return std::make_shared<make_shared_enabler>(filename, mode);
	}
	for (const std::string& p : _paths) {
		const std::string fullpath = p + filename;
		if (io::File(fullpath, FileMode::Read).exists()) {
			Log::debug("loading file %s from %s", filename.c_str(), p.c_str());
			return std::make_shared<make_shared_enabler>(fullpath, mode);
		}
	}
	return std::make_shared<make_shared_enabler>(_basePath + filename, mode);
}

std::string Filesystem::load(const char *filename, ...) {
	va_list ap;
	constexpr std::size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, filename);
	SDL_vsnprintf(text, bufSize, filename, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	return load(std::string(text));
}

std::string Filesystem::load(const std::string& filename) const {
	const io::FilePtr& f = open(filename);
	return f->load();
}

const std::string Filesystem::writePath(const char* name) const {
	return _homePath + name;
}

bool Filesystem::write(const std::string& filename, const uint8_t* content, size_t length) {
	io::File f(_homePath + filename, FileMode::Write);
	createDir(f.path());
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::write(const std::string& filename, const std::string& string) {
	const uint8_t* buf = reinterpret_cast<const uint8_t*>(string.c_str());
	return write(filename, buf, string.size());
}

bool Filesystem::syswrite(const std::string& filename, const uint8_t* content, size_t length) {
	io::File f(filename, FileMode::Write);
	createDir(f.path());
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::syswrite(const std::string& filename, const std::string& string) {
	const uint8_t* buf = reinterpret_cast<const uint8_t*>(string.c_str());
	return syswrite(filename, buf, string.size());
}

}
