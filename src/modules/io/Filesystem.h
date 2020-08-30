/**
 * @file
 */

#pragma once

#include "File.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Stack.h"
#include "core/collection/StringMap.h"
#include "core/Common.h"
#include <memory>
#include <uv.h>
#include <stdarg.h>
#include <SDL_stdinc.h>

namespace io {

typedef void (*FileWatcher)(const char* file);
using Paths = core::DynamicArray<core::String>;

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
	Paths _paths;

	core::Stack<core::String, 32> _dirStack;
	core::StringMap<uv_fs_event_t*> _watches;
	uv_loop_t *_loop = nullptr;

public:
	~Filesystem();

	bool init(const core::String& organisation, const core::String& appname);
	void shutdown();

	void update();

	const Paths& paths() const;

	bool registerPath(const core::String& path);

	bool unwatch(const core::String& path);
	bool unwatch(const io::FilePtr& file);
	bool watch(const core::String& path, FileWatcher watcher);
	bool watch(const io::FilePtr& file, FileWatcher watcher);

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

	/**
	 * @brief Returns a path where the given file can be saved.
	 */
	core::String writePath(const char* name) const;

	bool exists(const core::String& filename) const;

	struct DirEntry {
		core::String name;
		enum class Type : uint8_t {
			file,
			dir,
			unknown
		};
		Type type;
		uint64_t size;
	};

	bool list(const core::String& directory, core::DynamicArray<DirEntry>& entities, const core::String& filter = "") const;

	static bool isReadableDir(const core::String& name);
	static bool isRelativePath(const core::String& name);

	static core::String absolutePath(const core::String& path);

	/**
	 * @brief Changes the current working directory
	 * @see popDir()
	 * @see pushDir()
	 */
	static bool chdir(const core::String& directory);

	/**
	 * @brief changes the current working dir to the last pushed one
	 */
	bool popDir();

	/**
	 * @brief Push a working dir change onto the stack for later returning without knowing the origin
	 */
	bool pushDir(const core::String& directory);

	io::FilePtr open(const core::String& filename, FileMode mode = FileMode::Read) const;

	core::String load(CORE_FORMAT_STRING const char *filename, ...) CORE_PRINTF_VARARG_FUNC(2);

	core::String load(const core::String& filename) const;

	bool write(const core::String& filename, const uint8_t* content, size_t length);

	bool write(const core::String& filename, const core::String& string);

	/**
	 * @note The difference to the usual write() methods is that the given path is not put into the
	 * known file system structure of the application. It just uses the given name.
	 */
	bool syswrite(const core::String& filename, const uint8_t* content, size_t length) const;

	/**
	 * @note The difference to the usual write() methods is that the given path is not put into the
	 * known file system structure of the application. It just uses the given name.
	 */
	bool syswrite(const core::String& filename, const core::String& string) const;

	bool createDir(const core::String& dir, bool recursive = true) const;

	bool removeDir(const core::String& dir, bool recursive = false) const;
	bool removeFile(const core::String& file) const;
private:
	static bool _list(const core::String& directory, core::DynamicArray<DirEntry>& entities, const core::String& filter = "");
};

inline const Paths& Filesystem::paths() const {
	return _paths;
}

inline bool Filesystem::exists(const core::String& filename) const {
	return open(filename)->exists();
}

inline const core::String& Filesystem::basePath() const {
	return _basePath;
}

inline const core::String& Filesystem::homePath() const {
	return _homePath;
}

typedef std::shared_ptr<Filesystem> FilesystemPtr;

}
