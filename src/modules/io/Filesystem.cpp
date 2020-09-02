/**
 * @file
 */

#include "Filesystem.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/GameConfig.h"
#include "engine-config.h"
#include <SDL.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif
#include <uv.h>

namespace io {

Filesystem::~Filesystem() {
	shutdown();
}

bool Filesystem::init(const core::String& organisation, const core::String& appname) {
	core_assert(_loop == nullptr);
	_organisation = organisation;
	_appname = appname;

	_loop = new uv_loop_t;
	uv_loop_init(_loop);

	char *path = SDL_GetBasePath();
	if (path == nullptr) {
		_basePath = "";
	} else {
		_basePath = path;
		normalizePath(_basePath);
		SDL_free(path);
	}

	char *prefPath = SDL_GetPrefPath(_organisation.c_str(), _appname.c_str());
	if (prefPath == nullptr) {
		_homePath = "";
	} else {
		_homePath = prefPath;
		normalizePath(_homePath);
		SDL_free(prefPath);
		if (!createDir(_homePath, true)) {
			Log::error("Could not create home dir at: %s", _homePath.c_str());
			return false;
		}
	}

	core_assert_always(registerPath(_homePath));
#ifdef PKGDATADIR
	core_assert_always(registerPath(PKGDATADIR));
#endif
	const core::VarPtr& corePath = core::Var::get(cfg::CorePath, "", 0, "Specifies an additional filesystem search path");
	if (!corePath->strVal().empty()) {
		core_assert_always(registerPath(corePath->strVal()));
	}

	core_assert_always(registerPath(_basePath));

	core::Var::get(cfg::AppHomePath, _homePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);
	core::Var::get(cfg::AppBasePath, _basePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);

	return true;
}

bool Filesystem::removeFile(const core::String& file) const {
	if (file.empty()) {
		return false;
	}
	uv_fs_t req;
	return uv_fs_unlink(_loop, &req, file.c_str(), nullptr) == 0;
}

bool Filesystem::removeDir(const core::String& dir, bool recursive) const {
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

bool Filesystem::createDir(const core::String& dir, bool recursive) const {
	if (dir.empty()) {
		return false;
	}

	if (!recursive) {
		uv_fs_t req;
		// rwx+o, r+g
		const int retVal = uv_fs_mkdir(nullptr, &req, dir.c_str(), 0740, nullptr);
		if (retVal != 0 && req.result != UV_EEXIST) {
			Log::error("Failed to create dir '%s': %s", dir.c_str(), uv_strerror(retVal));
			uv_fs_req_cleanup(&req);
			return false;
		}
		uv_fs_req_cleanup(&req);
		return true;
	}

	core::String s = dir;
	if (s[s.size() - 1] != '/') {
		// force trailing / so we can handle everything in loop
		s += '/';
	}

	size_t pre = 0, pos;
	bool lastResult = false;
	while ((pos = s.find_first_of('/', pre)) != core::String::npos) {
		const core::String& dirpart = s.substr(0, pos++);
		pre = pos;
		if (dirpart.empty() || dirpart.last() == ':') {
			continue; // if leading / first time is 0 length
		}
		const char *dirc = dirpart.c_str();
		uv_fs_t req;
		const int retVal = uv_fs_mkdir(nullptr, &req, dirc, 0740, nullptr);
		if (retVal != 0 && req.result != UV_EEXIST) {
			Log::debug("Failed to create dir '%s': %s", dirc, uv_strerror(retVal));
			lastResult = false;
			uv_fs_req_cleanup(&req);
			continue;
		}
		lastResult = true;
		uv_fs_req_cleanup(&req);
	}
	return lastResult;
}

bool Filesystem::_list(const core::String& directory, core::DynamicArray<DirEntry>& entities, const core::String& filter) {
	uv_fs_t req;
	const int amount = uv_fs_scandir(nullptr, &req, directory.c_str(), 0, nullptr);
	if (amount < 0) {
		uv_fs_req_cleanup(&req);
		return false;
	}
	uv_dirent_t ent;
	core_memset(&ent, 0, sizeof(ent));
	while (uv_fs_scandir_next(&req, &ent) != UV_EOF) {
		DirEntry::Type type = DirEntry::Type::unknown;
		if (ent.type == UV_DIRENT_DIR) {
			type = DirEntry::Type::dir;
		} else if (ent.type == UV_DIRENT_FILE) {
			type = DirEntry::Type::file;
		} else if (ent.type == UV_DIRENT_UNKNOWN) {
			type = DirEntry::Type::unknown;
		} else if (ent.type == UV_DIRENT_LINK) {
			uv_fs_t linkReq;
			const core::String pointer = directory + "/" + ent.name;
			if (uv_fs_readlink(nullptr, &linkReq, pointer.c_str(), nullptr) != 0) {
				Log::debug("Could not resolve symlink %s", pointer.c_str());
				uv_fs_req_cleanup(&linkReq);
				continue;
			}
			const core::String symlink((const char*)linkReq.ptr);
			uv_fs_req_cleanup(&linkReq);
			if (!filter.empty()) {
				if (!core::string::matches(filter, symlink)) {
					continue;
				}
			}

			const core::String& fullPath = isRelativePath(symlink) ? directory + "/" + symlink : symlink;

			uv_fs_t statsReq;
			if (uv_fs_stat(nullptr, &statsReq, fullPath.c_str(), nullptr) != 0) {
				Log::debug("Could not stat file %s", fullPath.c_str());
				uv_fs_req_cleanup(&statsReq);
				continue;
			}
			const bool dir = (uv_fs_get_statbuf(&statsReq)->st_mode & S_IFDIR) != 0;
			entities.push_back(DirEntry{ent.name, dir ? DirEntry::Type::dir : DirEntry::Type::file, statsReq.statbuf.st_size});
			uv_fs_req_cleanup(&statsReq);
		} else {
			Log::debug("Unknown directory entry found: %s", ent.name);
			continue;
		}
		if (!filter.empty()) {
			if (!core::string::matches(filter.c_str(), ent.name)) {
				continue;
			}
		}
		uv_fs_t statsReq;
		const core::String fullPath = directory + "/" + ent.name;
		if (uv_fs_stat(nullptr, &statsReq, fullPath.c_str(), nullptr) != 0) {
			Log::warn("Could not stat file %s", fullPath.c_str());
		}
		entities.push_back(DirEntry{ent.name, type, statsReq.statbuf.st_size});
		uv_fs_req_cleanup(&statsReq);
	}
	uv_fs_req_cleanup(&req);
	return true;
}

bool Filesystem::list(const core::String& directory, core::DynamicArray<DirEntry>& entities, const core::String& filter) const {
	if (isRelativePath(directory)) {
		for (const core::String& p : _paths) {
			_list(p + directory, entities, filter);
		}
	} else {
		_list(directory, entities, filter);
	}
	return true;
}

void Filesystem::update() {
	uv_run(_loop, UV_RUN_NOWAIT);
}

bool Filesystem::chdir(const core::String& directory) {
	return uv_chdir(directory.c_str()) == 0;
}

void Filesystem::shutdown() {
	for (const auto& e : _watches) {
		uv_fs_event_stop((uv_fs_event_t*)e->value);
	}
	if (_loop != nullptr) {
		uv_run(_loop, UV_RUN_NOWAIT);
		if (uv_loop_close(_loop) != 0) {
			Log::error("Failed to close the filesystem event loop");
		}
		delete _loop;
		_loop = nullptr;
	}
	for (const auto& e : _watches) {
		delete e->value;
	}
	_watches.clear();
}

core::String Filesystem::absolutePath(const core::String& path) {
	uv_fs_t req;
	uv_fs_realpath(nullptr, &req, path.c_str(), nullptr);
	const char *c = (const char *)uv_fs_get_ptr(&req);
	if (c == nullptr) {
		uv_fs_req_cleanup(&req);
		return "";
	}
	core::String abspath = c;
	normalizePath(abspath);
	uv_fs_req_cleanup(&req);
	return abspath;
}

bool Filesystem::isReadableDir(const core::String& name) {
	uv_fs_t req;
	if (uv_fs_access(nullptr, &req, name.c_str(), F_OK, nullptr) != 0) {
		uv_fs_req_cleanup(&req);
		return false;
	}
	uv_fs_stat(nullptr, &req, name.c_str(), nullptr);
	const bool dir = (uv_fs_get_statbuf(&req)->st_mode & S_IFDIR) != 0;
	uv_fs_req_cleanup(&req);
	return dir;
}

bool Filesystem::isRelativePath(const core::String& name) {
	const size_t size = name.size();
#ifdef __WINDOWS__
	if (size < 3) {
		return true;
	}
	if (name[0] == '/') {
		return false;
	}
	// TODO: hm... not cool and most likely not enough
	return name[1] != ':';
#else
	if (size == 0) {
		return true;
	}
	return name[0] != '/';
#endif
}

bool Filesystem::registerPath(const core::String& path) {
	if (!core::string::endsWith(path, "/")) {
		Log::error("Failed to register data path: '%s'.", path.c_str());
		return false;
	}
	_paths.push_back(path);
	Log::info("Registered data path: '%s'", path.c_str());
	return true;
}

static bool closeFileWatchHandle(uv_fs_event_t* fshandle) {
	uv_fs_event_stop(fshandle);
	uv_handle_t* handle = (uv_handle_t*)fshandle;
	if (uv_handle_get_type(handle) == UV_UNKNOWN_HANDLE) {
		return false;
	}
	if (uv_is_closing(handle)) {
		return true;
	}
	uv_close(handle, [](uv_handle_t *handle) { delete handle; });
	return true;
}

bool Filesystem::unwatch(const core::String& path) {
	auto i = _watches.find(path);
	if (i == _watches.end()) {
		return false;
	}
	closeFileWatchHandle(i->value);
	_watches.erase(i);
	return true;
}

bool Filesystem::unwatch(const io::FilePtr& file) {
	return unwatch(file->name());
}

static void changeCallback (uv_fs_event_t *handle, const char *filename, int events, int status) {
	if ((events & UV_CHANGE) == 0) {
		return;
	}
	if (filename == nullptr) {
		return;
	}
	char path[1024];
	size_t size = sizeof(path) - 1;
	uv_fs_event_getpath(handle, path, &size);
	path[size] = '\0';

	FileWatcher watcherCallback = (FileWatcher)handle->data;
	watcherCallback(filename);

	// restart watching
	uv_fs_event_stop(handle);
	uv_fs_event_start(handle, changeCallback, path, 0U);
}

bool Filesystem::watch(const core::String& path, FileWatcher watcher) {
	unwatch(path);
	uv_fs_event_t* fsEvent = new uv_fs_event_t;
	if (uv_fs_event_init(_loop, fsEvent) != 0) {
		delete fsEvent;
		return false;
	}
	fsEvent->data = (void*)watcher;
	const int ret = uv_fs_event_start(fsEvent, changeCallback, path.c_str(), 0U);
	if (ret != 0) {
		if (!closeFileWatchHandle(fsEvent)) {
			delete fsEvent;
		}
		return false;
	}
	_watches.put(path, fsEvent);
	return true;
}

bool Filesystem::watch(const io::FilePtr& file, FileWatcher watcher) {
	return watch(file->name(), watcher);
}

bool Filesystem::popDir() {
	if (_dirStack.empty()) {
		return false;
	}
	const core::String& directory = _dirStack.top();
	chdir(directory);
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.pop();
	return true;
}

bool Filesystem::pushDir(const core::String& directory) {
	const bool changed = chdir(directory);
	if (!changed) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

io::FilePtr Filesystem::open(const core::String& filename, FileMode mode) const {
	if (mode == FileMode::SysWrite) {
		Log::debug("Use absolute path to open file %s for writing", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::Write) {
		return core::make_shared<io::File>(_homePath + filename, mode);
	} else if (mode == FileMode::SysRead) {
		return core::make_shared<io::File>(filename, mode);
	}
	io::File f(filename, FileMode::Read);
	if (f.exists()) {
		f.close();
		Log::debug("loading file %s from current working dir", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	}
	for (const core::String& p : _paths) {
		const core::String fullpath = p + filename;
		io::File fullFile(fullpath, FileMode::Read);
		if (fullFile.exists()) {
			fullFile.close();
			Log::debug("loading file %s from %s", filename.c_str(), p.c_str());
			return core::make_shared<io::File>(fullpath, mode);
		}
	}
	Log::debug("Use %s from %s", filename.c_str(), _basePath.c_str());
	return core::make_shared<io::File>(_basePath + filename, mode);
}

core::String Filesystem::load(const char *filename, ...) {
	va_list ap;
	constexpr size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, filename);
	SDL_vsnprintf(text, bufSize, filename, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	return load(core::String(text));
}

core::String Filesystem::load(const core::String& filename) const {
	const io::FilePtr& f = open(filename);
	return f->load();
}

core::String Filesystem::writePath(const char* name) const {
	return _homePath + name;
}

bool Filesystem::write(const core::String& filename, const uint8_t* content, size_t length) {
	const core::String& fullPath = _homePath + filename;
	const core::String path(core::string::extractPath(fullPath.c_str()));
	createDir(path, true);
	io::File f(fullPath, FileMode::Write);
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::write(const core::String& filename, const core::String& string) {
	const uint8_t* buf = reinterpret_cast<const uint8_t*>(string.c_str());
	return write(filename, buf, string.size());
}

bool Filesystem::syswrite(const core::String& filename, const uint8_t* content, size_t length) const {
	io::File f(filename, FileMode::SysWrite);
	createDir(f.path());
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::syswrite(const core::String& filename, const core::String& string) const {
	const uint8_t* buf = reinterpret_cast<const uint8_t*>(string.c_str());
	return syswrite(filename, buf, string.size());
}

}
