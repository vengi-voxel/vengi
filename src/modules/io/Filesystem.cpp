/**
 * @file
 */

#include "Filesystem.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h" // PKGDATADIR
#include "io/File.h"
#include "io/FileStream.h"
#include "io/FilesystemEntry.h"
#include "system/System.h"
#include <SDL.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace io {

Filesystem::~Filesystem() {
	shutdown();
}

bool Filesystem::init(const core::String &organisation, const core::String &appname) {
	_organisation = organisation;
	_appname = appname;

#ifdef __EMSCRIPTEN__
	EM_ASM(FS.mkdir('/libsdl'); FS.mount(IDBFS, {}, '/libsdl'); FS.syncfs(true, function(err) { assert(!err); }););
#endif

	char *path = SDL_GetBasePath();
	if (path != nullptr) {
		_basePath = core::Path(path);
		SDL_free(path);
	}

	char *prefPath = SDL_GetPrefPath(_organisation.c_str(), _appname.c_str());
	if (prefPath != nullptr) {
		_homePath = core::Path(prefPath);
		SDL_free(prefPath);
	}
	if (_homePath.empty()) {
		_homePath = core::Path("./");
	}
	const core::VarPtr &homePathVar =
		core::Var::get(cfg::AppHomePath, _homePath.c_str(), core::CV_READONLY | core::CV_NOPERSIST);

	_homePath = core::Path(homePathVar->strVal());
	if (!sysCreateDir(_homePath, true)) {
		Log::error("Could not create home dir at: %s", _homePath.c_str());
		return false;
	}

	core_assert_always(registerPath(core::Path(_homePath)));
	// this is a build system option that packagers could use to install
	// the application data into the proper system wide paths
#ifdef PKGDATADIR
	core_assert_always(registerPath(core::Path(PKGDATADIR)));
#endif

	// https://docs.appimage.org/packaging-guide/environment-variables.html
	const char *appImageDirectory = SDL_getenv("APPDIR");
	if (appImageDirectory != nullptr) {
		const core::String appDir = _organisation + "-" + _appname;
		const core::String appImagePath =
			core::string::sanitizeDirPath(core::string::path(appImageDirectory, "usr", "share", appDir));
		if (exists(appImagePath)) {
			core_assert_always(registerPath(core::Path(appImagePath)));
		}
	}

	// this cvar allows to change the application data directory at runtime - it has lower priority
	// as the backed-in PKGDATADIR (if defined) - and also lower priority as the home directory.
	const core::VarPtr &corePath =
		core::Var::get(cfg::CorePath, "", 0, "Specifies an additional filesystem search path - must end on /");
	if (!corePath->strVal().empty()) {
		if (exists(corePath->strVal())) {
			core_assert_always(registerPath(core::Path(corePath->strVal())));
		} else {
			Log::warn("%s '%s' does not exist", cfg::CorePath, corePath->strVal().c_str());
		}
	}

	if (!_basePath.empty()) {
		registerPath(core::Path(_basePath));
	}

	if (!initState(_state)) {
		Log::warn("Failed to initialize the filesystem state");
	}
	return true;
}

core::String Filesystem::sysFindBinary(const core::String &binaryName) const {
#ifdef _WIN32
	core::String binaryWithExtension = binaryName;
	binaryWithExtension += ".exe";
	core::Path binaryPath(binaryWithExtension);
#else
	core::Path binaryPath(binaryName);
#endif
	// Check current working directory
	if (fs_exists(binaryPath)) {
		return sysAbsolutePath(binaryPath.str());
	}

	// Check the directory of the current binary
	core::Path fullBinaryPath = _basePath.append(binaryName);
	if (fs_exists(fullBinaryPath)) {
		return sysAbsolutePath(fullBinaryPath.str());
	}

	// Check PATH environment variable
	if (const char *path = SDL_getenv("PATH")) {
		const char *pathSep =
#ifdef _WIN32
			";";
#else
			":";
#endif
		core::DynamicArray<core::String> paths;
		core::string::splitString(path, paths, pathSep);
		for (const auto &p : paths) {
			core::Path binPath(p);
			binPath.append(binaryPath);
			if (fs_exists(binPath)) {
				return binPath.str();
			}
		}
	}
	return "";
}

const core::DynamicArray<ThisPCEntry> Filesystem::sysOtherPaths() const {
	return _state._thisPc;
}

core::String Filesystem::sysSpecialDir(FilesystemDirectories dir) const {
	return _state._directories[dir];
}

bool Filesystem::sysRemoveFile(const core::Path &file) const {
	if (file.empty()) {
		Log::error("Can't delete file: No path given");
		return false;
	}
	return fs_unlink(file);
}

bool Filesystem::sysRemoveDir(const core::Path &dir, bool recursive) const {
	if (dir.empty()) {
		Log::error("Can't delete dir: No path given");
		return false;
	}

	if (!recursive) {
		return fs_rmdir(dir);
	}
	// TODO: implement me
	return false;
}

bool Filesystem::sysCreateDir(const core::Path &dir, bool recursive) const {
	if (dir.empty()) {
		return false;
	}

	if (!recursive) {
		if (!fs_mkdir(dir)) {
			Log::error("Failed to create dir '%s'", dir.c_str());
			return false;
		}
		return true;
	}

	// force trailing / so we can handle everything in loop
	core::String s = core::string::sanitizeDirPath(dir.str());

	size_t pre = 0, pos;
	bool lastResult = false;
	while ((pos = s.find_first_of('/', pre)) != core::String::npos) {
		const core::String &dirpart = s.substr(0, pos++);
		pre = pos;
		if (dirpart.empty() || dirpart.last() == ':') {
			continue; // if leading / first time is 0 length
		}
		if (!fs_mkdir(core::Path(dirpart))) {
			Log::debug("Failed to create dir '%s'", dirpart.c_str());
			lastResult = false;
			continue;
		}
		lastResult = true;
	}
	return lastResult;
}

bool Filesystem::_list(const core::Path &directory, core::DynamicArray<FilesystemEntry> &entities,
					   const core::String &filter, int depth) {
	const core::DynamicArray<FilesystemEntry> &entries = fs_scandir(directory);
	Log::debug("Found %i entries in %s", (int)entries.size(), directory.c_str());
	for (FilesystemEntry entry : entries) {
		normalizePath(entry.name);
		core::Path fullPath = directory.append(entry.name);
		entry.fullPath = fullPath.str();
		if (entry.type == FilesystemEntry::Type::link) {
			core::Path symlink = fs_readlink(core::Path(entry.fullPath));
			if (symlink.empty()) {
				Log::debug("Could not resolve symlink %s", entry.fullPath.c_str());
				continue;
			}
			if (!filter.empty()) {
				if (!core::string::fileMatchesMultiple(symlink.c_str(), filter.c_str())) {
					Log::trace("File %s doesn't match filter %s", symlink.c_str(), filter.c_str());
					continue;
				}
			}

			if (symlink.isRelativePath()) {
				entry.fullPath = core::string::path(directory.str(), symlink.str());
			} else {
				entry.fullPath = symlink.str();
			}
		} else if (entry.type == FilesystemEntry::Type::dir && depth > 0) {
			_list(core::Path(entry.fullPath), entities, filter, depth - 1);
		} else {
			if (!filter.empty()) {
				if (!core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
					Log::trace("Entity %s doesn't match filter %s", entry.name.c_str(), filter.c_str());
					continue;
				}
			}
		}
		if (!fs_stat(core::Path(entry.fullPath), entry)) {
			Log::debug("Could not stat file %s", entry.fullPath.c_str());
		}
		entities.push_back(entry);
	}
	if (entries.empty()) {
		Log::debug("No files found in %s", directory.c_str());
		return false;
	}
	return true;
}

bool Filesystem::list(const core::String &directory, core::DynamicArray<FilesystemEntry> &entities,
					  const core::String &filter, int depth) const {
	core::Path path(directory);
	if (path.isRelativePath()) {
		const core::Path cwd = sysCurrentDir();
		for (const core::Path &p : _paths) {
			const core::Path &fullDir = p.append(path);
			if (fullDir == cwd) {
				continue;
			}
			_list(fullDir, entities, filter, depth);
		}
		if (path.empty()) {
			_list(cwd, entities, filter, depth);
		}
	} else {
		_list(path, entities, filter, depth);
	}
	return true;
}

bool Filesystem::sysChdir(const core::Path &directory) {
	Log::debug("Change current working dir to %s", directory.c_str());
	return fs_chdir(directory);
}

void Filesystem::shutdown() {
#ifdef __EMSCRIPTEN__
	EM_ASM(FS.syncfs(true, function(err){}););
#endif
}

core::String Filesystem::sysAbsolutePath(const core::String &path) const {
	core::Path abspath = fs_realpath(core::Path(path));
	if (abspath.empty()) {
		for (const core::Path &p : registeredPaths()) {
			const core::Path &fullPath = p.append(path);
			abspath = fs_realpath(fullPath);
			if (!abspath.empty()) {
				return abspath.str();
			}
		}
		Log::error("Failed to get absolute path for '%s'", path.c_str());
		return "";
	}
	return abspath.str();
}

bool Filesystem::sysIsHidden(const core::Path &name) {
	return fs_hidden(name);
}

bool Filesystem::sysIsReadableDir(const core::String &name) {
	core::Path path(name);
	if (!fs_exists(path)) {
		Log::trace("%s doesn't exist", name.c_str());
		return false;
	}

	FilesystemEntry entry;
	if (!fs_stat(path, entry)) {
		Log::trace("Could not stat '%s'", name.c_str());
		return false;
	}
	Log::trace("Found type %i for '%s'", (int)entry.type, name.c_str());
	return entry.type == FilesystemEntry::Type::dir;
}

bool Filesystem::registerPath(const core::Path &path) {
	_paths.push_back(path);
	Log::debug("Registered data path: '%s'", path.c_str());
	return true;
}

core::Path Filesystem::sysCurrentDir() const {
	return core::Path(fs_cwd());
}

bool Filesystem::sysPopDir() {
	if (_dirStack.empty()) {
		return false;
	}
	_dirStack.pop();
	if (_dirStack.empty()) {
		return false;
	}
	const core::Path &directory = _dirStack.top();
	Log::trace("change current dir to %s", directory.c_str());
	if (!sysChdir(directory)) {
		return false;
	}
	return true;
}

bool Filesystem::sysPushDir(const core::Path &directory) {
	if (_dirStack.empty()) {
		core::Path cwd = sysCurrentDir();
		_dirStack.push(cwd);
	}
	if (!sysChdir(directory)) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

// TODO: case insensitive search should be possible - see searchPathFor()
io::FilePtr Filesystem::open(const core::String &filename, FileMode mode) const {
	const core::Path path(filename);
	core_assert_msg(!_homePath.empty(), "Filesystem is not yet initialized");
	if (sysIsReadableDir(filename)) {
		Log::debug("%s is a directory - skip this", filename.c_str());
		return core::make_shared<io::File>("", mode);
	}
	if (mode == FileMode::SysWrite) {
		Log::debug("Use absolute path to open file %s for writing", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::SysRead && fs_exists(path)) {
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::Write) {
		if (!path.isRelativePath()) {
			Log::error("%s can't get opened in write mode", filename.c_str());
			return core::make_shared<io::File>("", mode);
		}
		const core::Path fullpath = _homePath.append(filename);
		sysCreateDir(fullpath.dirname(), true);
		return core::make_shared<io::File>(_homePath.append(filename), mode);
	}
	FileMode openmode = mode;
	if (openmode == FileMode::ReadNoHome) {
		openmode = FileMode::Read;
	}
	for (const core::Path &p : _paths) {
		if (mode == FileMode::ReadNoHome && p == _homePath) {
			Log::debug("Skip reading home path");
			continue;
		}
		core::Path fullpath = p.append(filename);
		if (fs_exists(fullpath)) {
			Log::debug("loading file %s from %s", filename.c_str(), p.c_str());
			return core::make_shared<io::File>(core::move(fullpath), openmode);
		}
		if (p.isRelativePath()) {
			for (const core::Path &s : _paths) {
				if (s.isRelativePath() || s == p) {
					continue;
				}
				const core::Path fullrelpath = s + p + filename;
				if (fs_exists(fullrelpath)) {
					Log::debug("loading file %s from %s%s", filename.c_str(), s.c_str(), p.c_str());
					return core::make_shared<io::File>(core::move(fullrelpath), openmode);
				}
			}
		}
	}
	if (fs_exists(path)) {
		Log::debug("loading file '%s'", filename.c_str());
		return core::make_shared<io::File>(filename, openmode);
	}
	if (!path.isRelativePath()) {
		Log::debug("'%s' not found", filename.c_str());
		return core::make_shared<io::File>("", openmode);
	}
	Log::debug("Use %s from %s", filename.c_str(), _basePath.c_str());
	return core::make_shared<io::File>(_basePath.append(filename), openmode);
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

core::Path Filesystem::homeWritePath(const core::String &name) const {
	return _homePath.append(name);
}

long Filesystem::homeWrite(const core::String &filename, io::ReadStream &stream) {
	const core::Path fullPath = _homePath.append(filename);
	const core::Path path = fullPath.dirname();
	sysCreateDir(path, true);
	io::File f(fullPath, FileMode::Write);
	long written = f.write(stream);
	f.close();
	return written;
}

bool Filesystem::homeWrite(const core::String &filename, const uint8_t *content, size_t length) {
	const core::Path fullPath = _homePath.append(filename);
	const core::Path path = fullPath.dirname();
	sysCreateDir(path, true);
	io::File f(fullPath, FileMode::Write);
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::homeWrite(const core::String &filename, const core::String &string) {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return homeWrite(filename, buf, string.size());
}

bool Filesystem::sysWrite(const core::String &filename, const uint8_t *content, size_t length) const {
	io::File f(filename, FileMode::SysWrite);
	if (!sysCreateDir(f.dir())) {
		Log::error("Failed to write to %s: Could not create the directory", filename.c_str());
		return false;
	}
	f.open(FileMode::SysWrite);
	return f.write(content, length) == static_cast<long>(length);
}

long Filesystem::sysWrite(const core::String &filename, io::ReadStream &stream) const {
	io::File f(filename, FileMode::SysWrite);
	if (!sysCreateDir(f.dir())) {
		Log::error("Failed to write to %s: Could not create the directory", filename.c_str());
		return false;
	}
	f.open(FileMode::SysWrite);
	return f.write(stream);
}

bool Filesystem::sysWrite(const core::String &filename, const core::String &string) const {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return sysWrite(filename, buf, string.size());
}

core::String searchPathFor(const FilesystemPtr &filesystem, const core::String &path, const core::String &filename) {
	if (filename.empty()) {
		Log::warn("No filename given to perform lookup in '%s'", path.c_str());
		return "";
	}
	core::DynamicArray<core::String> tokens;
	core::string::splitString(path, tokens, "/");
	while (!tokens.empty()) {
		if (filesystem->sysIsReadableDir(tokens[0])) {
			Log::trace("readable dir: %s", tokens[0].c_str());
			break;
		}
		Log::trace("not a readable dir: %s", tokens[0].c_str());
		tokens.erase(0);
		if (tokens.empty()) {
			break;
		}
	}
	core::String relativePath;
	for (const core::String &t : tokens) {
		relativePath = core::string::path(relativePath, t);
	}
	core::DynamicArray<io::FilesystemEntry> entities;
	const core::String abspath = filesystem->sysAbsolutePath(relativePath);
	filesystem->list(abspath, entities);
	Log::trace("Found %i entries in %s", (int)entities.size(), abspath.c_str());
	auto predicate = [&](const io::FilesystemEntry &e) { return core::string::iequals(e.name, filename); };
	auto iter = core::find_if(entities.begin(), entities.end(), predicate);
	if (iter == entities.end()) {
		Log::debug("Could not find %s in '%s'", filename.c_str(), abspath.c_str());
		for (const auto &e : entities) {
			Log::trace("* %s", e.name.c_str());
		}
		return "";
	}
	Log::debug("Found %s in %s", iter->name.c_str(), relativePath.c_str());
	return core::string::path(abspath, iter->name);
}

} // namespace io
