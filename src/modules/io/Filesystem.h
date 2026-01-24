/**
 * @file
 */

#pragma once

#include "File.h"
#include "FilesystemEntry.h"
#include "core/Path.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Stack.h"
#include "core/Common.h"
#include "core/collection/StringMap.h"
#include "io/Stream.h"

namespace io {

using Paths = core::DynamicArray<core::String>;

enum FilesystemDirectories {
	FS_Dir_Download,
	FS_Dir_Desktop,
	FS_Dir_Documents,
	FS_Dir_Pictures,
	FS_Dir_Public,
	FS_Dir_Fonts,
	FS_Dir_Recent,
	FS_Dir_Cloud,

	FS_Dir_Max
};

struct ThisPCEntry {
	core::String name;
	core::String path;
};

struct FilesystemState {
	core::String _directories[FilesystemDirectories::FS_Dir_Max];
	core::DynamicArray<ThisPCEntry> _thisPc;
};


// perform platform specific initialization
extern bool initState(FilesystemState& state);

/**
 * @brief Hide platform specific details about the io handling for files.
 *
 * @note You can load file synchronous or asynchronous with a callback.
 */
class Filesystem {
private:
	core::String _organisation;
	core::String _appname;

	/**
	 * This is the directory where the application was run from, which is probably
	 * the installation directory or the current working directory. In case the
	 * binary is a symlink, it it resolved.
	 */
	core::String _basePath;
	core::String _homePath;
	FilesystemState _state;
	Paths _paths;

	core::Stack<core::Path, 32> _dirStack;

	static bool _list(const core::String& directory, core::DynamicArray<FilesystemEntry>& entities, const core::String& filter = "", int depth = 0);

public:
	~Filesystem();

	bool init(const core::String& organisation, const core::String& appname);
	void shutdown();

	const Paths& registeredPaths() const;
	/**
	 * @param[in] path If this is a relative path the filesystem will append this relative path to all
	 * known search paths when trying to find a file.
	 */
	bool registerPath(const core::String& path);

	/**
	 * @brief Get the path where the application resides.
	 *
	 * Get the "base path". This is the directory where the application was run
	 * from, which is probably the installation directory, and may or may not
	 * be the process's current working directory.
	 * @note Ends with path separator
	 */
	const core::String& basePath() const;
	/**
	 * @brief The path where the application can store data
	 * @note Ends with path separator
	 */
	const core::String& homePath() const;

	bool exists(const core::String& filename) const;

	/**
	 * @brief List all entities in a directory that match the given optional filter
	 * @param directory The directory to list
	 * @param entities The list of directory entities that were found
	 * @param[in] filter Wildcard for filtering the returned entities. Separated by a comma. Example *.vox,*.qb,*.mcr
	 * @return @c true if the directory could get opened
	 */
	bool list(const core::String& directory, core::DynamicArray<FilesystemEntry>& entities, const core::String& filter = "", int depth = 0) const;

	io::FilePtr open(const core::String& filename, FileMode mode = FileMode::Read) const;
	io::FilePtr open(const core::Path& path, FileMode mode = FileMode::Read) const {
		return open(path.str(), mode);
	}

	core::String filePath(const core::String& filename) const;

	core::String load(CORE_FORMAT_STRING const char *filename, ...) CORE_PRINTF_VARARG_FUNC(2);
	core::String load(const core::String& filename) const;

	// HOME PATH HANDLING

	/**
	 * @brief Returns a path where the given file can be saved.
	 */
	core::String homeWritePath(const core::String &name) const;

	bool homeWrite(const core::String& filename, const uint8_t* content, size_t length);
	long homeWrite(const core::String& filename, io::ReadStream &stream);
	bool homeWrite(const core::String& filename, const core::String& string);

	// SYS PATH HANDLING

	/**
	 * @brief changes the current working dir to the last pushed one
	 */
	bool sysPopDir();
	/**
	 * @brief Push a working dir change onto the stack for later returning without knowing the origin
	 */
	bool sysPushDir(const core::Path& directory);

	core::String sysSpecialDir(FilesystemDirectories dir) const;
	const core::DynamicArray<ThisPCEntry> sysOtherPaths() const;

	static bool sysIsWriteable(const core::String& path) {
		return sysIsWriteable(core::Path(path));
	}
	static bool sysIsWriteable(const core::Path& path);

	static bool sysExists(const core::String& path) {
		return sysExists(core::Path(path));
	}
	static bool sysExists(const core::Path& path);

	core::String sysFindBinary(const core::String &binaryName) const;

	/**
	 * @brief The current working directory without a tailing /
	 */
	static core::String sysCurrentDir();

	static bool sysIsReadableDir(const core::String& name);
	static bool sysIsReadableDir(const core::Path& name) {
		return sysIsReadableDir(name.str());
	}
	static bool sysIsHidden(const core::String &name);
	static bool sysIsHidden(const core::Path &name) {
		return sysIsHidden(name.str());
	}
	static bool sysIsRelativePath(const core::String &name);
	static bool sysIsRelativePath(const core::Path &name) {
		return sysIsRelativePath(name.str());
	}
	core::String sysAbsolutePath(const core::String& path) const;

	/**
	 * @brief Changes the current working directory
	 * @see popDir()
	 * @see pushDir()
	 */
	static bool sysChdir(const core::String& directory);
	static bool sysChdir(const core::Path& directory);

	/**
	 * @note The difference to the usual write() methods is that the given path is not put into the
	 * known file system structure of the application. It just uses the given name.
	 */
	static bool sysWrite(const core::String& filename, const uint8_t* content, size_t length);
	static long sysWrite(const core::String& filename, io::ReadStream &stream);

	/**
	 * @note The difference to the usual write() methods is that the given path is not put into the
	 * known file system structure of the application. It just uses the given name.
	 */
	static bool sysWrite(const core::String& filename, const core::String& string);

	/**
	 * @brief This will create the directory without taking the write path into account. BEWARE!
	 * @param dir The full path to the directory or relative to the current working dir of your app.
	 */
	static bool sysCreateDir(const core::String& dir, bool recursive = true);
	static bool sysCreateDir(const core::Path& dir, bool recursive = true) {
		return sysCreateDir(dir.str(), recursive);
	}

	/**
	 * @brief This will remove the directory without taking the write path into account. BEWARE!
	 * @param dir The full path to the directory or relative to the current working dir of your app.
	 */
	static bool sysRemoveDir(const core::String& dir, bool recursive = false);
	static bool sysRemoveDir(const core::Path& dir, bool recursive = false) {
		return sysRemoveDir(dir.str(), recursive);
	}
	/**
	 * @brief This will remove the file without taking the write path into account. BEWARE!
	 * @param file The full path to the file or relative to the current working dir of your app.
	 */
	static bool sysRemoveFile(const core::String& file);
	static bool sysRemoveFile(const core::Path& file) {
		return sysRemoveFile(file.str());
	}
};

inline const Paths& Filesystem::registeredPaths() const {
	return _paths;
}

inline const core::String& Filesystem::basePath() const {
	return _basePath;
}

inline const core::String& Filesystem::homePath() const {
	return _homePath;
}

typedef core::SharedPtr<Filesystem> FilesystemPtr;

}
