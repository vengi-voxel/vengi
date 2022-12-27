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
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "io/File.h"
#include "io/FilesystemEntry.h"
#include <SDL.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif

namespace io {

extern bool fs_mkdir(const char *path);
extern bool fs_rmdir(const char *path);
extern bool fs_unlink(const char *path);
extern bool fs_exists(const char *path);
extern bool fs_chdir(const char *path);
extern core::String fs_realpath(const char *path);
extern bool fs_stat(const char *path, FilesystemEntry &entry);
extern core::DynamicArray<FilesystemEntry> fs_scandir(const char *path);
extern core::String fs_readlink(const char *path);
extern core::String fs_cwd();

Filesystem::~Filesystem() {
	shutdown();
}

bool Filesystem::init(const core::String &organisation, const core::String &appname) {
	_organisation = organisation;
	_appname = appname;

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
		Log::error("Can't delete file: No path given");
		return false;
	}
	return fs_unlink(file.c_str());
}

bool Filesystem::removeDir(const core::String &dir, bool recursive) const {
	if (dir.empty()) {
		Log::error("Can't delete dir: No path given");
		return false;
	}

	if (!recursive) {
		return fs_rmdir(dir.c_str());
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
	const core::DynamicArray<FilesystemEntry> &entries = fs_scandir(directory.c_str());
	if (entries.empty()) {
		Log::debug("No files found in %s", directory.c_str());
		return false;
	}
	for (FilesystemEntry entry : entries) {
		normalizePath(entry.name);
		core::String fullPath = core::string::path(directory, entry.name);
		if (entry.type == FilesystemEntry::Type::link) {
			const core::String pointer = core::string::path(directory, entry.name);
			core::String symlink = fs_readlink(pointer.c_str());
			normalizePath(symlink);
			if (symlink.empty()) {
				Log::debug("Could not resolve symlink %s", pointer.c_str());
				continue;
			}
			if (!filter.empty()) {
				if (!core::string::fileMatchesMultiple(symlink.c_str(), filter.c_str())) {
					Log::debug("File %s doesn't match filter %s", symlink.c_str(), filter.c_str());
					continue;
				}
			}

			fullPath = isRelativePath(symlink) ? core::string::path(directory, symlink) : symlink;
		} else {
			if (!filter.empty()) {
				if (!core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
					Log::debug("Entity %s doesn't match filter %s", entry.name.c_str(), filter.c_str());
					continue;
				}
			}

		}
		if (!fs_stat(fullPath.c_str(), entry)) {
			Log::debug("Could not stat file %s", fullPath.c_str());
		}
		entities.push_back(entry);
	}
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

bool Filesystem::chdir(const core::String &directory) {
	return fs_chdir(directory.c_str());
}

void Filesystem::shutdown() {
}

core::String Filesystem::absolutePath(const core::String &path) {
	core::String abspath = fs_realpath(path.c_str());
	if (abspath.empty()) {
		Log::error("Failed to get absolute path for '%s'", path.c_str());
		return "";
	}
	normalizePath(abspath);
	return abspath;
}

bool Filesystem::isReadableDir(const core::String &name) {
	if (!fs_exists(name.c_str())) {
		Log::trace("%s doesn't exist", name.c_str());
		return false;
	}

	FilesystemEntry entry;
	if (!fs_stat(name.c_str(), entry)) {
		Log::trace("Could not stat '%s'", name.c_str());
		return false;
	}
	Log::trace("Found type %i for '%s'", (int)entry.type, name.c_str());
	return entry.type == FilesystemEntry::Type::dir;
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

core::String Filesystem::currentDir() const {
	core::String cwd = fs_cwd();
	normalizePath(cwd);
	return cwd;
}

bool Filesystem::popDir() {
	if (_dirStack.empty()) {
		return false;
	}
	_dirStack.pop();
	if (_dirStack.empty()) {
		return false;
	}
	const core::String &directory = _dirStack.top();
	Log::trace("change current dir to %s", directory.c_str());
	if (!chdir(directory)) {
		return false;
	}
	return true;
}

bool Filesystem::pushDir(const core::String &directory) {
	if (_dirStack.empty()) {
		core::String cwd = currentDir();
		_dirStack.push(cwd);
	}
	if (!chdir(directory)) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

// TODO: case insensitive search should be possible
io::FilePtr Filesystem::open(const core::String &filename, FileMode mode) const {
	if (isReadableDir(filename)) {
		Log::debug("%s is a directory - skip this", filename.c_str());
		return core::make_shared<io::File>("", mode);
	}
	if (mode == FileMode::SysWrite) {
		Log::debug("Use absolute path to open file %s for writing", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::SysRead) {
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::Write) {
		if (!isRelativePath(filename)) {
			Log::error("%s can't get opened in write mode", filename.c_str());
			return core::make_shared<io::File>("", mode);
		}
		return core::make_shared<io::File>(_homePath + filename, mode);
	}
	io::File f(filename, FileMode::Read);
	if (f.exists()) {
		f.close();
		Log::debug("loading file %s from current working dir", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	}
	if (!isRelativePath(filename)) {
		Log::debug("%s not found", filename.c_str());
		return core::make_shared<io::File>("", mode);
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
	if (!createDir(f.path())) {
		Log::error("Failed to write to %s: Could not create the directory", filename.c_str());
		return false;
	}
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::syswrite(const core::String &filename, const core::String &string) const {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return syswrite(filename, buf, string.size());
}

core::String searchPathFor(const FilesystemPtr& filesystem, const core::String &path, const core::String &filename) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString(path, tokens, "/");
	while (!tokens.empty()) {
		if (filesystem->isReadableDir(tokens[0])) {
			break;
		}
		tokens.erase(0);
	}
	core::String relativePath;
	for (const core::String &t : tokens) {
		relativePath += t;
		relativePath += "/";
	}
	core::DynamicArray<io::FilesystemEntry> entities;
	const core::String abspath = filesystem->absolutePath(relativePath);
	filesystem->list(abspath, entities);
	Log::debug("Found %i entries in %s", (int)entities.size(), relativePath.c_str());
	auto predicate = [&] (const io::FilesystemEntry &e) {
		return core::string::iequals(e.name, filename);
	};
	auto iter = core::find_if(entities.begin(), entities.end(), predicate);
	if (iter == entities.end()) {
		Log::debug("Could not find %s in %s", filename.c_str(), relativePath.c_str());
		return "";
	}
	Log::debug("Found %s in %s", iter->name.c_str(), relativePath.c_str());
	return core::string::path(abspath, iter->name);
}

} // namespace io
