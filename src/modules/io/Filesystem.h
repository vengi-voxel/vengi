/**
 * @file
 */

#pragma once

#include <memory>
#include "core/ThreadPool.h"
#include "File.h"
#include <stack>
#include <uv.h>
#include <unordered_map>
#include <stdarg.h>
#include <SDL.h>

namespace io {

typedef void (*FileWatcher)(const char* file);

/**
 * @brief Hide platform specific details about the io handling for files.
 *
 * @note You can load file synchronous or asynchronous with a callback.
 */
class Filesystem {
private:
	core::ThreadPool _threadPool;
	std::string _organisation;
	std::string _appname;

	std::string _basePath;
	std::string _homePath;

	std::stack<std::string> _dirStack;
	std::unordered_map<std::string, uv_fs_event_t*> _watches;
	uv_loop_t *_loop = nullptr;

public:
	Filesystem();
	~Filesystem();

	void init(const std::string& organisation, const std::string& appname);
	void shutdown();

	void update();

	bool unwatch(const std::string& path);
	bool watch(const std::string& path, FileWatcher watcher);

	/**
	 * @brief Get the path where the application resides.
	 *
	 * Get the "base path". This is the directory where the application was run
	 * from, which is probably the installation directory, and may or may not
	 * be the process's current working directory.
	 */
	const std::string& basePath() const;
	/**
	 * @brief The path where the application can store data
	 */
	const std::string& homePath() const;

	bool exists(const std::string& filename) const;

	struct DirEntry {
		std::string name;
		enum class Type : uint8_t {
			file,
			dir,
			unknown
		};
		Type type;
	};

	bool list(const std::string& directory, std::vector<DirEntry>& entities, const std::string& filter = "") const;

	bool isRelativeFilename(const std::string& name) const;

	bool chdir(const std::string& directory);

	bool popDir();

	bool pushDir(const std::string& directory);

	io::FilePtr open(const std::string& filename, FileMode mode = FileMode::Read) const;

	std::string load(SDL_PRINTF_FORMAT_STRING const char *filename, ...) SDL_PRINTF_VARARG_FUNC(2);

	std::string load(const std::string& filename) const;
	/**
	 * @brief Loads a file asynchronously and executes the given @c CompleteHandle once some result is available.
	 */
	template<class CompleteHandle>
	void loadAsync(const std::string& filename, CompleteHandle&& completeHandle) {
		_threadPool.enqueue([=]() {
			const io::FilePtr& f = std::make_shared<io::File>(filename, FileMode::Read);
			completeHandle(f);
		});
	}

	bool write(const std::string& filename, const uint8_t* content, size_t length);

	bool write(const std::string& filename, const std::string& string);

	bool syswrite(const std::string& filename, const uint8_t* content, size_t length);

	bool syswrite(const std::string& filename, const std::string& string);

	bool createDir(const std::string& dir, bool recursive = true) const;

	bool removeDir(const std::string& dir, bool recursive = false) const;
};

inline bool Filesystem::exists(const std::string& filename) const {
	return open(filename)->exists();
}

inline const std::string& Filesystem::basePath() const {
	return _basePath;
}

inline const std::string& Filesystem::homePath() const {
	return _homePath;
}

typedef std::shared_ptr<Filesystem> FilesystemPtr;

}
