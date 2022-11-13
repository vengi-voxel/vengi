/**
 * @file
 */

#include "Filesystem.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "engine-config.h"
#include <SDL.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif
// TODO: get rid of libuv
#include <uv.h>

namespace io {

extern bool fs_mkdir(const char *path);
extern bool fs_remove(const char *path);
extern bool fs_exists(const char *path);

Filesystem::~Filesystem() {
	shutdown();
}

bool Filesystem::init(const core::String &organisation, const core::String &appname) {
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
	if (prefPath != nullptr) {
		_homePath = prefPath;
		SDL_free(prefPath);
	}
	if (_homePath.empty()) {
		_homePath = "./";
	}
	const core::VarPtr &homePathVar =
		core::Var::get(cfg::AppHomePath, _homePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);

	_homePath = homePathVar->strVal();
	normalizePath(_homePath);
	if (!createDir(_homePath, true)) {
		Log::error("Could not create home dir at: %s", _homePath.c_str());
		return false;
	}

	core_assert_always(registerPath(_homePath));
	// this is a build system option that packagers could use to install
	// the application data into the proper system wide paths
#ifdef PKGDATADIR
	core_assert_always(registerPath(PKGDATADIR));
#endif
	// this cvar allows to change the application data directory at runtime - it has lower priority
	// as the backed-in PKGDATADIR (if defined) - and also lower priority as the home directory.
	const core::VarPtr &corePath =
		core::Var::get(cfg::CorePath, "", 0, "Specifies an additional filesystem search path - must end on /");
	if (!corePath->strVal().empty()) {
		core_assert_always(registerPath(corePath->strVal()));
	}

	if (!_basePath.empty()) {
		registerPath(_basePath);
	}

	if (!initState(_state)) {
		Log::warn("Failed to initialize the filesystem state");
	}
	return true;
}

core::String Filesystem::specialDir(FilesystemDirectories dir) const {
	return _state._directories[dir];
}

bool Filesystem::removeFile(const core::String &file) const {
	if (file.empty()) {
		return false;
	}
	return fs_remove(file.c_str());
}

bool Filesystem::removeDir(const core::String &dir, bool recursive) const {
	if (dir.empty()) {
		return false;
	}

	if (!recursive) {
		return fs_remove(dir.c_str());
	}
	// TODO: implement me
	return false;
}

bool Filesystem::createDir(const core::String &dir, bool recursive) const {
	if (dir.empty()) {
		return false;
	}

	if (!recursive) {
		if (!fs_mkdir(dir.c_str())) {
			Log::error("Failed to create dir '%s'", dir.c_str());
			return false;
		}
		return true;
	}

	// force trailing / so we can handle everything in loop
	core::String s = core::string::sanitizeDirPath(dir);

	size_t pre = 0, pos;
	bool lastResult = false;
	while ((pos = s.find_first_of('/', pre)) != core::String::npos) {
		const core::String &dirpart = s.substr(0, pos++);
		pre = pos;
		if (dirpart.empty() || dirpart.last() == ':') {
			continue; // if leading / first time is 0 length
		}
		const char *dirc = dirpart.c_str();
		if (!fs_mkdir(dirc)) {
			Log::debug("Failed to create dir '%s'", dirc);
			lastResult = false;
			continue;
		}
		lastResult = true;
	}
	return lastResult;
}

bool Filesystem::_list(const core::String &directory, core::DynamicArray<FilesystemEntry> &entities,
					   const core::String &filter) {
	uv_fs_t req;
	const int amount = uv_fs_scandir(nullptr, &req, directory.c_str(), 0, nullptr);
	if (amount < 0) {
		uv_fs_req_cleanup(&req);
		Log::debug("No files found in %s", directory.c_str());
		return false;
	}
	uv_dirent_t ent;
	core_memset(&ent, 0, sizeof(ent));
	while (uv_fs_scandir_next(&req, &ent) != UV_EOF) {
		FilesystemEntry::Type type = FilesystemEntry::Type::unknown;
		if (ent.type == UV_DIRENT_DIR) {
			type = FilesystemEntry::Type::dir;
		} else if (ent.type == UV_DIRENT_FILE) {
			type = FilesystemEntry::Type::file;
		} else if (ent.type == UV_DIRENT_UNKNOWN) {
			type = FilesystemEntry::Type::unknown;
		} else if (ent.type == UV_DIRENT_LINK) {
			uv_fs_t linkReq;
			const core::String pointer = core::string::path(directory, ent.name);
			if (uv_fs_readlink(nullptr, &linkReq, pointer.c_str(), nullptr) != 0) {
				Log::debug("Could not resolve symlink %s", pointer.c_str());
				uv_fs_req_cleanup(&linkReq);
				continue;
			}
			const core::String symlink((const char *)linkReq.ptr);
			uv_fs_req_cleanup(&linkReq);
			if (!filter.empty()) {
				if (!core::string::fileMatchesMultiple(symlink.c_str(), filter.c_str())) {
					Log::debug("File %s doesn't match filter %s", symlink.c_str(), filter.c_str());
					continue;
				}
			}

			const core::String &fullPath = isRelativePath(symlink) ? core::string::path(directory, symlink) : symlink;

			uv_fs_t statsReq;
			if (uv_fs_stat(nullptr, &statsReq, fullPath.c_str(), nullptr) != 0) {
				Log::debug("Could not stat file %s", fullPath.c_str());
				uv_fs_req_cleanup(&statsReq);
				continue;
			}
			const bool dir = (uv_fs_get_statbuf(&statsReq)->st_mode & S_IFDIR) != 0;
			const uint64_t mtimeMillis =
				(uint64_t)statsReq.statbuf.st_mtim.tv_sec * 1000 + statsReq.statbuf.st_mtim.tv_nsec / 1000000;
			entities.push_back(FilesystemEntry{ent.name, dir ? FilesystemEntry::Type::dir : FilesystemEntry::Type::file,
										statsReq.statbuf.st_size, mtimeMillis});
			uv_fs_req_cleanup(&statsReq);
		} else {
			Log::debug("Unknown directory entry found: %s", ent.name);
			continue;
		}
		if (!filter.empty()) {
			if (!core::string::fileMatchesMultiple(ent.name, filter.c_str())) {
				Log::debug("Entity %s doesn't match filter %s", ent.name, filter.c_str());
				continue;
			}
		}
		uv_fs_t statsReq;
		const core::String fullPath = core::string::path(directory, ent.name);
		if (uv_fs_stat(nullptr, &statsReq, fullPath.c_str(), nullptr) != 0) {
			Log::warn("Could not stat file %s", fullPath.c_str());
		}
		const uint64_t mtimeMillis =
			(uint64_t)statsReq.statbuf.st_mtim.tv_sec * 1000 + statsReq.statbuf.st_mtim.tv_nsec / 1000000;
		entities.push_back(FilesystemEntry{ent.name, type, statsReq.statbuf.st_size, mtimeMillis});
		uv_fs_req_cleanup(&statsReq);
	}
	uv_fs_req_cleanup(&req);
	return true;
}

bool Filesystem::list(const core::String &directory, core::DynamicArray<FilesystemEntry> &entities,
					  const core::String &filter) const {
	if (isRelativePath(directory)) {
		for (const core::String &p : _paths) {
			const core::String fullDir = p + directory;
			Log::debug("List %s in %s", filter.c_str(), fullDir.c_str());
			_list(fullDir, entities, filter);
		}
	} else {
		_list(directory, entities, filter);
	}
	return true;
}

void Filesystem::update() {
	uv_run(_loop, UV_RUN_NOWAIT);
}

bool Filesystem::chdir(const core::String &directory) {
	return uv_chdir(directory.c_str()) == 0;
}

void Filesystem::shutdown() {
	if (_loop != nullptr) {
		uv_run(_loop, UV_RUN_NOWAIT);
		if (uv_loop_close(_loop) != 0) {
			Log::error("Failed to close the filesystem event loop");
		}
		delete _loop;
		_loop = nullptr;
	}
}

core::String Filesystem::absolutePath(const core::String &path) {
	uv_fs_t req;
	const int retVal = uv_fs_realpath(nullptr, &req, path.c_str(), nullptr);
	if (retVal != 0) {
		Log::error("Failed to get absolute path for '%s': %s", path.c_str(), uv_strerror(retVal));
		return "";
	}
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

bool Filesystem::isReadableDir(const core::String &name) {
	if (!fs_exists(name.c_str())) {
		return false;
	}

	uv_fs_t req;
	uv_fs_stat(nullptr, &req, name.c_str(), nullptr);
	const bool dir = (uv_fs_get_statbuf(&req)->st_mode & S_IFDIR) != 0;
	uv_fs_req_cleanup(&req);
	return dir;
}

bool Filesystem::isRelativePath(const core::String &name) {
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

bool Filesystem::registerPath(const core::String &path) {
	if (!core::string::endsWith(path, "/")) {
		Log::error("Failed to register data path: '%s' - it must end on /.", path.c_str());
		return false;
	}
	_paths.push_back(path);
	Log::debug("Registered data path: '%s'", path.c_str());
	return true;
}

bool Filesystem::popDir() {
	if (_dirStack.empty()) {
		return false;
	}
	const core::String &directory = _dirStack.top();
	chdir(directory);
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.pop();
	return true;
}

bool Filesystem::pushDir(const core::String &directory) {
	const bool changed = chdir(directory);
	if (!changed) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

io::FilePtr Filesystem::open(const core::String &filename, FileMode mode) const {
	if (isReadableDir(filename)) {
		Log::debug("%s is a directory - skip this", filename.c_str());
		return core::make_shared<io::File>("", mode);
	}
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
	for (const core::String &p : _paths) {
		const core::String fullpath = core::string::path(p, filename);
		io::File fullFile(fullpath, FileMode::Read);
		if (fullFile.exists()) {
			fullFile.close();
			Log::debug("loading file %s from %s", filename.c_str(), p.c_str());
			return core::make_shared<io::File>(fullpath, mode);
		}
		if (isRelativePath(p)) {
			for (const core::String &s : _paths) {
				if (s == p) {
					continue;
				}
				const core::String fullrelpath = core::string::path(s, p, filename);
				io::File fullrelFile(fullrelpath, FileMode::Read);
				if (fullrelFile.exists()) {
					fullrelFile.close();
					Log::debug("loading file %s from %s%s", filename.c_str(), s.c_str(), p.c_str());
					return core::make_shared<io::File>(fullrelpath, mode);
				}
			}
		}
	}
	Log::debug("Use %s from %s", filename.c_str(), _basePath.c_str());
	return core::make_shared<io::File>(core::string::path(_basePath, filename), mode);
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

core::String Filesystem::load(const core::String &filename) const {
	const io::FilePtr &f = open(filename);
	return f->load();
}

core::String Filesystem::writePath(const char *name) const {
	return _homePath + name;
}

bool Filesystem::write(const core::String &filename, const uint8_t *content, size_t length) {
	const core::String &fullPath = _homePath + filename;
	const core::String path(core::string::extractPath(fullPath.c_str()));
	createDir(path, true);
	io::File f(fullPath, FileMode::Write);
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::write(const core::String &filename, const core::String &string) {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return write(filename, buf, string.size());
}

bool Filesystem::syswrite(const core::String &filename, const uint8_t *content, size_t length) const {
	io::File f(filename, FileMode::SysWrite);
	createDir(f.path());
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::syswrite(const core::String &filename, const core::String &string) const {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return syswrite(filename, buf, string.size());
}

} // namespace io
