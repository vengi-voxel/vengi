/**
 * @file
 */

#pragma once

#include <memory>
#include "core/ThreadPool.h"
#include "File.h"

namespace io {

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
public:
	Filesystem();

	void init(const std::string& organisation, const std::string& appname);

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

	bool list(const std::string& directory, std::vector<DirEntry>& entries, const std::string& filter = "") const;

	io::FilePtr open(const std::string& filename, FileMode mode = FileMode::Read) const;

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

	bool createDir(const std::string& dir) const;
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
