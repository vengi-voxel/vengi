/**
 * @file
 */

#include "Filesystem.h"
#include "app/I18N.h"
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
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include "system/emscripten_browser_file.h"
#include <emscripten.h>
#endif

namespace io {

Filesystem::~Filesystem() {
	shutdown();
}

bool Filesystem::init(const core::String &organisation, const core::String &appname) {
	_organisation = organisation;
	_appname = appname;

	const char *path = SDL_GetBasePath();
	if (path == nullptr) {
		_basePath = core::Path();
	} else {
		core::String base = path;
		normalizePath(base);
		_basePath = core::Path(core::move(base));
	}

	char *prefPath = nullptr;
#if defined(__ANDROID__)
	if (SDL_GetAndroidJNIEnv() != nullptr)
#endif
	{
		prefPath = SDL_GetPrefPath(_organisation.c_str(), _appname.c_str());
	}
	if (prefPath != nullptr) {
		core::String home = prefPath;
		normalizePath(home);
		_homePath = core::Path(core::move(home));
		SDL_free(prefPath);
	}
	if (_homePath.empty()) {
		_homePath = fs_cwd();
		if (_homePath.empty()) {
			_homePath = core::Path("./");
		}
	}
	const core::VarDef appHomePath(cfg::AppHomePath, _homePath.str(), N_("Home Path"),
								   N_("The home directory path for the application"),
								   core::CV_READONLY | core::CV_NOPERSIST);
	const core::VarPtr &homePathVar = core::Var::registerVar(appHomePath);

	core::String home = homePathVar->strVal();
	normalizePath(home);
	_homePath = core::Path(core::string::sanitizeDirPath(home));
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
	const core::VarDef corePathDef(cfg::CorePath, "", "Specifies an additional filesystem search path - must end on /", 0);
	const core::VarPtr &corePath = core::Var::registerVar(corePathDef);
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

core::Path Filesystem::sysFindBinary(const core::String &binaryName) const {
	core::String binaryWithExtension = binaryName;
#ifdef _WIN32
	binaryWithExtension += ".exe";
#endif
	const core::Path binaryPath(binaryWithExtension);
	// Check current working directory
	if (fs_exists(binaryPath)) {
		return sysAbsolutePath(binaryPath);
	}

	// Check the directory of the current binary
	const core::Path fullBinaryPath = _basePath.append(binaryWithExtension);
	if (fs_exists(fullBinaryPath)) {
		return sysAbsolutePath(fullBinaryPath);
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
			const core::Path binPath = core::Path(p).append(binaryWithExtension);
			if (fs_exists(binPath)) {
				return binPath;
			}
		}
	}
	return core::Path();
}

const core::DynamicArray<ThisPCEntry> Filesystem::sysOtherPaths() const {
	return _state._thisPc;
}

core::String Filesystem::sysSpecialDir(FilesystemDirectories dir) const {
	return _state._directories[dir];
}

bool Filesystem::sysRenamePath(const core::String &from, const core::String &to) {
	if (from.empty() || to.empty()) {
		Log::error("Can't rename path: No source or destination given");
		return false;
	}
	if (!SDL_RenamePath(from.c_str(), to.c_str())) {
		Log::error("Failed to rename %s to %s: %s", from.c_str(), to.c_str(), SDL_GetError());
		return false;
	}
	return true;
}

bool Filesystem::sysRemoveFile(const core::Path &file) {
	if (file.empty()) {
		Log::error("Can't delete file: No path given");
		return false;
	}
	return fs_unlink(file);
}

bool Filesystem::sysRemoveDir(const core::Path &dir, bool recursive) {
	if (dir.empty()) {
		Log::error("Can't delete dir: No path given");
		return false;
	}

	if (!recursive) {
		return fs_rmdir(dir);
	}
	// TODO: implement recursive directory deletion
	return false;
}

bool Filesystem::sysCreateDir(const core::Path &dir, bool recursive) {
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
		entry.fullPath = directory.append(entry.name).str();
		if (entry.isLink()) {
			core::String symlink = fs_readlink(core::Path(entry.fullPath)).str();
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

			entry.fullPath = sysIsRelativePath(symlink) ? directory.append(symlink).str() : symlink;
		} else if (entry.isDirectory() && depth > 0) {
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

bool Filesystem::list(const core::Path &directory, core::DynamicArray<FilesystemEntry> &entities,
					  const core::String &filter, int depth) const {
	if (!sysIsRelativePath(directory)) {
		_list(directory, entities, filter, depth);
		return true;
	}

	// A registered folder such as "scripts/" is itself a search root. Listing "scripts" must scan that
	// folder, not "scripts/scripts". The same directory can also be reached through the base path.
	const core::Path cwd = sysCurrentDir();
	core::DynamicArray<core::Path> seen;
	auto visit = [&](const core::Path &dir) {
		if (dir.empty()) {
			return;
		}
		// "scripts/" and "<cwd>/scripts" are the same folder once the search root is relative
		core::Path absolute = dir;
		if (sysIsRelativePath(absolute)) {
			absolute = cwd.append(absolute);
		}
		for (const core::Path &known : seen) {
			if (known == absolute) {
				return;
			}
		}
		seen.push_back(absolute);
		_list(absolute, entities, filter, depth);
	};

	for (const core::Path &p : _paths) {
		core::Path fullDir;
		if (directory.empty() || (sysIsRelativePath(p) && p == directory)) {
			fullDir = p;
		} else {
			fullDir = p.append(directory);
		}
		if (fullDir == cwd) {
			continue;
		}
		visit(fullDir);
	}
	if (directory.empty()) {
		visit(cwd);
	}
	return true;
}

core::String Filesystem::filePath(const core::String& filename) const {
	const io::FilePtr &file = open(filename);
	const core::String name = file->name();
	file->close();
	return name;
}

bool Filesystem::sysChdir(const core::Path &directory) {
	Log::debug("Change current working dir to %s", directory.c_str());
	return fs_chdir(directory);
}

void Filesystem::shutdown() {
}

core::Path Filesystem::sysAbsolutePath(const core::Path &path) const {
	core::String abspath = fs_realpath(path).str();
	if (abspath.empty()) {
		for (const core::Path &p : registeredPaths()) {
			abspath = fs_realpath(p.append(path)).str();
			if (!abspath.empty()) {
				normalizePath(abspath);
				return core::Path(core::move(abspath));
			}
		}
		Log::debug("Failed to get absolute path for '%s'", path.c_str());
		return core::Path();
	}
	normalizePath(abspath);
	return core::Path(core::move(abspath));
}

bool Filesystem::sysIsHidden(const core::Path &name) {
	return fs_hidden(name);
}

bool Filesystem::sysExists(const core::Path& path) {
	return fs_exists(path);
}

bool Filesystem::sysIsWriteable(const core::Path& path) {
	if (!sysExists(path)) {
		core::Path parent = path.dirname();
		if (!sysExists(parent)) {
			return false;
		}
		return fs_writeable(parent);
	}
	return fs_writeable(path);
}

bool Filesystem::sysIsReadableDir(const core::String &name) {
	if (!fs_exists(core::Path(name))) {
		Log::trace("%s doesn't exist", name.c_str());
		return false;
	}

	FilesystemEntry entry;
	if (!fs_stat(core::Path(name), entry)) {
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

bool Filesystem::registerPath(const core::Path &path) {
	if (!core::string::endsWith(path.str(), '/')) {
		Log::error("Failed to register data path: '%s' - it must end on /.", path.c_str());
		return false;
	}
	_paths.push_back(path);
	Log::debug("Registered data path: '%s'", path.c_str());
	return true;
}

core::Path Filesystem::sysCurrentDir() {
	core::String cwd = fs_cwd().str();
	normalizePath(cwd);
	return core::Path(core::move(cwd));
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
		core::Path cwd(sysCurrentDir());
		_dirStack.push(cwd);
	}
	if (!sysChdir(directory)) {
		return false;
	}
	Log::trace("change current dir to %s", directory.c_str());
	_dirStack.push(directory);
	return true;
}

bool Filesystem::exists(const core::String& filename) const {
	const core::Path file(filename);
	if (sysIsReadableDir(file)) {
		return true;
	}
	for (const core::Path &p : _paths) {
		const core::Path fullpath = p.append(file);
		if (fs_exists(fullpath)) {
			return true;
		}
		if (sysIsRelativePath(p)) {
			for (const core::Path &s : _paths) {
				// scripts/ is attached to absolute search roots, not to other relative folders
				if (sysIsRelativePath(s) || s == p) {
					continue;
				}
				const core::Path fullrelpath = s + p + file;
				if (fs_exists(fullrelpath)) {
					return true;
				}
			}
		}
	}
	if (fs_exists(file)) {
		return true;
	}
	if (!sysIsRelativePath(file)) {
		return false;
	}
	return fs_exists(_basePath.append(file));
}

io::FilePtr Filesystem::open(const core::Path &filename, FileMode mode) const {
	core_assert_msg(!_homePath.empty(), "Filesystem is not yet initialized");
	if (sysIsReadableDir(filename)) {
		Log::trace("%s is a directory - skip this", filename.c_str());
		return core::make_shared<io::File>("", mode);
	}
	if (mode == FileMode::SysWrite) {
		Log::trace("Use absolute path to open file %s for writing", filename.c_str());
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::SysRead && fs_exists(filename)) {
		return core::make_shared<io::File>(filename, mode);
	} else if (mode == FileMode::Write) {
		if (!sysIsRelativePath(filename)) {
			Log::error("%s can't get opened in write mode", filename.c_str());
			return core::make_shared<io::File>("", mode);
		}
		const core::Path fullpath = _homePath.append(filename);
		sysCreateDir(core::Path(core::string::extractDir(fullpath.str())), true);
		return core::make_shared<io::File>(fullpath, mode);
	}
	FileMode openmode = mode;
	if (openmode == FileMode::ReadNoHome) {
		openmode = FileMode::Read;
	}
	for (const core::Path &p : _paths) {
		if (mode == FileMode::ReadNoHome && p == _homePath) {
			Log::trace("Skip reading home path");
			continue;
		}
		const core::Path fullpath = p.append(filename);
		if (fs_exists(fullpath)) {
			Log::trace("loading file %s from %s for mode %i", filename.c_str(), p.c_str(), (int)openmode);
			return core::make_shared<io::File>(fullpath, openmode);
		}
		if (sysIsRelativePath(p)) {
			for (const core::Path &s : _paths) {
				// scripts/ is attached to absolute search roots, not to other relative folders
				if (sysIsRelativePath(s) || s == p) {
					continue;
				}
				const core::Path fullrelpath = s + p + filename;
				if (fs_exists(fullrelpath)) {
					Log::trace("loading file %s from %s%s for mode %i", filename.c_str(), s.c_str(), p.c_str(), (int)openmode);
					return core::make_shared<io::File>(fullrelpath, openmode);
				}
			}
		}
	}
	if (fs_exists(filename)) {
		Log::trace("loading file '%s'", filename.c_str());
		return core::make_shared<io::File>(filename, openmode);
	}
	if (!sysIsRelativePath(filename)) {
		Log::trace("'%s' not found for mode %i", filename.c_str(), (int)openmode);
		return core::make_shared<io::File>("", openmode);
	}
	Log::trace("Use %s from %s for mode %i", filename.c_str(), _basePath.c_str(), (int)openmode);
	return core::make_shared<io::File>(_basePath.append(filename), openmode);
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
	core::String fullPath = core::string::sanitizePath(_homePath.append(name).str());
	if (!core::string::startsWith(fullPath, _homePath.str())) {
		Log::error("Refused path that escapes the home directory: %s (homePath: \"%s\", fullPath: \"%s\")", name.c_str(), _homePath.c_str(), fullPath.c_str());
		return core::String::Empty;
	}
	return fullPath;
}

long Filesystem::homeWrite(const core::String &filename, io::ReadStream &stream) {
	const core::String &fullPath = homeWritePath(filename);
	const core::String path(core::string::extractDir(fullPath));
	sysCreateDir(path, true);
	io::File f(fullPath, FileMode::Write);
	long written = f.write(stream);
	f.close();
	return written;
}

bool Filesystem::homeWrite(const core::String &filename, const uint8_t *content, size_t length) {
	const core::String &fullPath = homeWritePath(filename);
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
	const core::Path dir = f.dir();
	if (!dir.empty() && !sysCreateDir(dir)) {
		Log::error("Failed to write to %s: Could not create the directory", filename.c_str());
		return false;
	}
	f.open(FileMode::SysWrite);
	return f.write(content, length) == static_cast<long>(length);
}

long Filesystem::sysWrite(const core::String &filename, io::ReadStream &stream) {
	io::File f(filename, FileMode::SysWrite);
	const core::Path dir = f.dir();
	if (!dir.empty() && !sysCreateDir(dir)) {
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

bool Filesystem::sysOfferDownload(const core::String &filename) {
#ifdef __EMSCRIPTEN__
	if (filename.empty()) {
		return false;
	}
	io::File f(filename, FileMode::SysRead);
	if (!f.validHandle()) {
		Log::error("Failed to offer download for '%s': could not open file", filename.c_str());
		return false;
	}
	uint8_t *buf = nullptr;
	const int len = f.read((void **)&buf);
	if (len <= 0 || buf == nullptr) {
		delete[] buf;
		Log::error("Failed to offer download for '%s': empty or unreadable file", filename.c_str());
		return false;
	}
	core::String downloadName = core::string::extractFilenameWithExtension(filename);
	if (downloadName.empty()) {
		downloadName = filename;
	}
	emscripten_browser_file::download(downloadName.c_str(), "application/octet-stream", buf, (size_t)len);
	delete[] buf;
	return true;
#else
	(void)filename;
	return false;
#endif
}

} // namespace io
