/**
 * @file
 */

#include "Filesystem.h"
#include "core/Assert.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Path.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h" // PKGDATADIR, PKGDATABASEDIR
#include "io/File.h"
#include "io/FileStream.h"
#include "io/FilesystemEntry.h"
#include "system/System.h"
#include "core/sdl/SDLSystem.h"
#if defined(_WIN32) || defined(__CYGWIN__)
#include <locale>
#else
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
#if defined(_WIN32) || defined(__CYGWIN__)
	// https://github.com/tronkko/dirent/issues/39
	std::locale::global(std::locale("LC_CTYPE=.utf8"));
#endif
	_organisation = organisation;
	_appname = appname;

#ifdef __EMSCRIPTEN__
	EM_ASM({
		try {
			// Try to create the directory
			if (!FS.analyzePath('/libsdl').exists) {
				FS.mkdir('/libsdl');
			}
			// Mount the filesystem
			FS.mount(IDBFS, {}, '/libsdl');
			// Synchronize the filesystem
			FS.syncfs(true, function (err) {
				if (err) {
					console.error('Filesystem sync error:', err);
				}
			});
		} catch (e) {
			console.error('Filesystem initialization error:', e);
		}
	});
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
	const char *path = SDL_GetBasePath();
	if (path == nullptr) {
		_basePath = "";
	} else {
		_basePath = path;
		normalizePath(_basePath);
	}
#else
	char *path = SDL_GetBasePath();
	if (path == nullptr) {
		_basePath = "";
	} else {
		_basePath = path;
		normalizePath(_basePath);
		SDL_free(path);
	}
#endif

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
	if (!sysCreateDir(_homePath, true)) {
		Log::error("Could not create home dir at: %s", _homePath.c_str());
		return false;
	}

	const core::String appDir = _organisation + "-" + _appname;

	core_assert_always(registerPath(_homePath));
	// this is a build system option that packagers could use to install
	// the application data into the proper system wide paths
#ifdef PKGDATADIR
	core_assert_always(registerPath(PKGDATADIR));
#endif
#ifdef PKGDATABASEDIR
	const core::String pkgDataBaseDir = core::string::sanitizeDirPath(core::string::path(PKGDATABASEDIR, appDir));
	core_assert_always(registerPath(pkgDataBaseDir));
#endif

	// https://docs.appimage.org/packaging-guide/environment-variables.html
	const char *appImageDirectory = SDL_getenv("APPDIR");
	if (appImageDirectory != nullptr) {
		const core::String appImagePath = core::string::sanitizeDirPath(core::string::path(appImageDirectory, "usr", "share", appDir));
		if (exists(appImagePath)) {
			core_assert_always(registerPath(appImagePath));
		}
	}

	// this cvar allows to change the application data directory at runtime - it has lower priority
	// as the backed-in PKGDATADIR (if defined) - and also lower priority as the home directory.
	const core::VarPtr &corePath =
		core::Var::get(cfg::CorePath, "", 0, "Specifies an additional filesystem search path - must end on /");
	if (!corePath->strVal().empty()) {
		if (exists(corePath->strVal())) {
			core_assert_always(registerPath(corePath->strVal()));
		} else {
			Log::warn("%s '%s' does not exist", cfg::CorePath, corePath->strVal().c_str());
		}
	}

	if (!_basePath.empty()) {
		registerPath(_basePath);
	}

	if (!initState(_state)) {
		Log::warn("Failed to initialize the filesystem state");
	}
	return true;
}

core::String Filesystem::sysFindBinary(const core::String &binaryName) const {
	core::String binaryWithExtension = binaryName;
#ifdef _WIN32
	binaryWithExtension += ".exe";
#endif
	// Check current working directory
	if (fs_exists(binaryWithExtension.c_str())) {
		return sysAbsolutePath(binaryWithExtension);
	}

	// Check the directory of the current binary
	core::String binaryPath = core::string::path(_basePath, binaryWithExtension);
	if (fs_exists(binaryPath.c_str())) {
		return sysAbsolutePath(binaryPath);
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
			const core::String binPath = core::string::path(p, binaryWithExtension);
			if (fs_exists(binPath.c_str())) {
				return binPath;
			}
		}
	}
	return core::String::Empty;
}

const core::DynamicArray<ThisPCEntry> Filesystem::sysOtherPaths() const {
	return _state._thisPc;
}

core::String Filesystem::sysSpecialDir(FilesystemDirectories dir) const {
	return _state._directories[dir];
}

bool Filesystem::sysRemoveFile(const core::String &file) {
	if (file.empty()) {
		Log::error("Can't delete file: No path given");
		return false;
	}
	return fs_unlink(file.c_str());
}

bool Filesystem::sysRemoveDir(const core::String &dir, bool recursive) {
	if (dir.empty()) {
		Log::error("Can't delete dir: No path given");
		return false;
	}

	if (!recursive) {
		return fs_rmdir(dir.c_str());
	}
	// TODO: implement recursive directory deletion
	return false;
}

bool Filesystem::sysCreateDir(const core::String &dir, bool recursive) {
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
					   const core::String &filter, int depth) {
	const core::DynamicArray<FilesystemEntry> &entries = fs_scandir(directory.c_str());
	Log::debug("Found %i entries in %s", (int)entries.size(), directory.c_str());
	for (FilesystemEntry entry : entries) {
		normalizePath(entry.name);
		entry.fullPath = core::string::path(directory, entry.name);
		if (entry.isLink()) {
			core::String symlink = fs_readlink(entry.fullPath.c_str());
			normalizePath(symlink);
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

			entry.fullPath = sysIsRelativePath(symlink) ? core::string::path(directory, symlink) : symlink;
		} else if (entry.isDirectory() && depth > 0) {
			_list(entry.fullPath, entities, filter, depth - 1);
		} else {
			if (!filter.empty()) {
				if (!core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
					Log::trace("Entity %s doesn't match filter %s", entry.name.c_str(), filter.c_str());
					continue;
				}
			}
		}
		if (!fs_stat(entry.fullPath.c_str(), entry)) {
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
	if (sysIsRelativePath(directory)) {
		const core::String cwd = sysCurrentDir();
		for (const core::String &p : _paths) {
			const core::String fullDir = core::string::path(p, directory);
			if (core::string::isSamePath(fullDir, cwd)) {
				continue;
			}
			_list(fullDir, entities, filter, depth);
		}
		if (directory.empty()) {
			_list(cwd, entities, filter, depth);
		}
	} else {
		_list(directory, entities, filter, depth);
	}
	return true;
}

bool Filesystem::sysChdir(const core::String &directory) {
	Log::debug("Change current working dir to %s", directory.c_str());
	return fs_chdir(directory.c_str());
}

void Filesystem::shutdown() {
#ifdef __EMSCRIPTEN__
	EM_ASM({
		try {
			// Synchronize changes before unmounting
			FS.syncfs(function (err) {
				if (err) {
					console.error('Filesystem sync error:', err);
				}
			});
			// Unmount the filesystem
			FS.unmount('/libsdl');
		} catch (e) {
			console.error('Filesystem shutdown error:', e);
		}
	});
#endif
}

core::String Filesystem::sysAbsolutePath(const core::String &path) const {
	core::String abspath = fs_realpath(path.c_str());
	if (abspath.empty()) {
		for (const core::String &p : registeredPaths()) {
			const core::String &fullPath = core::string::path(p, path);
			abspath = fs_realpath(fullPath.c_str());
			if (!abspath.empty()) {
				normalizePath(abspath);
				return abspath;
			}
		}
		Log::debug("Failed to get absolute path for '%s'", path.c_str());
		return core::String::Empty;
	}
	normalizePath(abspath);
	return abspath;
}

bool Filesystem::sysIsHidden(const core::String &name) {
	return fs_hidden(name.c_str());
}

bool Filesystem::sysExists(const core::Path& path) {
	return fs_exists(path.c_str());
}

bool Filesystem::sysIsWriteable(const core::Path& path) {
	if (!sysExists(path)) {
		core::Path parent = path.dirname();
		if (!sysExists(parent)) {
			return false;
		}
		return fs_writeable(parent.c_str());
	}
	return fs_writeable(path.c_str());
}

bool Filesystem::sysIsReadableDir(const core::String &name) {
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

bool Filesystem::sysIsRelativePath(const core::String &name) {
	const size_t size = name.size();
#if defined(_WIN32) || defined(__CYGWIN__)
	if (size < 2) {
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
	if (!core::string::endsWith(path, '/')) {
		Log::error("Failed to register data path: '%s' - it must end on /.", path.c_str());
		return false;
	}
	_paths.push_back(path);
	Log::debug("Registered data path: '%s'", path.c_str());
	return true;
}

core::String Filesystem::sysCurrentDir() {
	core::String cwd = fs_cwd();
	normalizePath(cwd);
	return cwd;
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
	if (!sysChdir(directory.toNativePath())) {
		return false;
	}
	return true;
}

bool Filesystem::sysPushDir(const core::Path &directory) {
	if (_dirStack.empty()) {
		core::Path cwd(sysCurrentDir());
		_dirStack.push(cwd);
	}
	if (!sysChdir(directory.toNativePath())) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

bool Filesystem::exists(const core::String& filename) const {
	if (sysIsReadableDir(filename)) {
		return true;
	}
	return open(filename)->exists();
}

io::FilePtr Filesystem::open(const core::String &filename, FileMode mode) const {
	core_assert_msg(!_homePath.empty(), "Filesystem is not yet initialized");
	if (sysIsReadableDir(filename)) {
		Log::trace("%s is a directory - skip this", filename.c_str());
		return core::make_shared<io::File>("", mode);
	}
	if (mode == FileMode::SysWrite) {
		Log::trace("Use absolute path to open file %s for writing", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::SysRead && fs_exists(filename.c_str())) {
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::Write) {
		if (!sysIsRelativePath(filename)) {
			Log::error("%s can't get opened in write mode", filename.c_str());
			return core::make_shared<io::File>("", mode);
		}
		sysCreateDir(core::string::path(_homePath, core::string::extractDir(filename)), true);
		return core::make_shared<io::File>(core::string::path(_homePath, filename), mode);
	}
	FileMode openmode = mode;
	if (openmode == FileMode::ReadNoHome) {
		openmode = FileMode::Read;
	}
	for (const core::String &p : _paths) {
		if (mode == FileMode::ReadNoHome && p == _homePath) {
			Log::trace("Skip reading home path");
			continue;
		}
		core::String fullpath = core::string::path(p, filename);
		if (fs_exists(fullpath.c_str())) {
			Log::trace("loading file %s from %s for mode %i", filename.c_str(), p.c_str(), (int)openmode);
			return core::make_shared<io::File>(core::move(fullpath), openmode);
		}
		if (sysIsRelativePath(p)) {
			for (const core::String &s : _paths) {
				if (core::string::isSamePath(s, p)) {
					continue;
				}
				core::String fullrelpath = core::string::path(s, p, filename);
				if (fs_exists(fullrelpath.c_str())) {
					Log::trace("loading file %s from %s%s for mode %i", filename.c_str(), s.c_str(), p.c_str(), (int)openmode);
					return core::make_shared<io::File>(core::move(fullrelpath), openmode);
				}
			}
		}
	}
	if (fs_exists(filename.c_str())) {
		Log::trace("loading file '%s'", filename.c_str());
		return core::make_shared<io::File>(filename, openmode);
	}
	if (!sysIsRelativePath(filename)) {
		Log::trace("'%s' not found for mode %i", filename.c_str(), (int)openmode);
		return core::make_shared<io::File>("", openmode);
	}
	Log::trace("Use %s from %s for mode %i", filename.c_str(), _basePath.c_str(), (int)openmode);
	return core::make_shared<io::File>(core::string::path(_basePath, filename), openmode);
}

core::String Filesystem::load(const char *filename, ...) {
	va_list ap;
	constexpr size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, filename);
	int len = SDL_vsnprintf(text, bufSize, filename, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	if (len >= 0) {
		return load(core::String(text, len));
	}
	return core::String::Empty;
}

core::String Filesystem::load(const core::String &filename) const {
	const io::FilePtr &f = open(filename);
	return f->load();
}

core::String Filesystem::homeWritePath(const core::String &name) const {
	return core::string::path(_homePath, name);
}

long Filesystem::homeWrite(const core::String &filename, io::ReadStream &stream) {
	const core::String &fullPath = core::string::path(_homePath, filename);
	const core::String path(core::string::extractDir(fullPath));
	sysCreateDir(path, true);
	io::File f(fullPath, FileMode::Write);
	long written = f.write(stream);
	f.close();
	return written;
}

bool Filesystem::homeWrite(const core::String &filename, const uint8_t *content, size_t length) {
	const core::String &fullPath = core::string::path(_homePath, filename);
	const core::String path(core::string::extractDir(fullPath));
	sysCreateDir(path, true);
	io::File f(fullPath, FileMode::Write);
	return f.write(content, length) == static_cast<long>(length);
}

bool Filesystem::homeWrite(const core::String &filename, const core::String &string) {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return homeWrite(filename, buf, string.size());
}

bool Filesystem::sysWrite(const core::String &filename, const uint8_t *content, size_t length) {
	io::File f(filename, FileMode::SysWrite);
	if (!sysCreateDir(f.dir())) {
		Log::error("Failed to write to %s: Could not create the directory", filename.c_str());
		return false;
	}
	f.open(FileMode::SysWrite);
	return f.write(content, length) == static_cast<long>(length);
}

long Filesystem::sysWrite(const core::String &filename, io::ReadStream &stream) {
	io::File f(filename, FileMode::SysWrite);
	if (!sysCreateDir(f.dir())) {
		Log::error("Failed to write to %s: Could not create the directory", filename.c_str());
		return false;
	}
	f.open(FileMode::SysWrite);
	return f.write(stream);
}

bool Filesystem::sysWrite(const core::String &filename, const core::String &string) {
	const uint8_t *buf = reinterpret_cast<const uint8_t *>(string.c_str());
	return sysWrite(filename, buf, string.size());
}

} // namespace io
