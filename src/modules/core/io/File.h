/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <memory>
#include "IOResource.h"

struct SDL_RWops;

namespace io {

class Filesystem;

enum class FileMode {
	Read, Write
};

extern void normalizePath(std::string& str);

/**
 * @brief Wrapper for file based io.
 *
 * @see FileSystem
 */
class File : public IOResource {
	friend class FileStream;
	friend class Filesystem;
protected:
	SDL_RWops* _file;
	std::string _rawPath;
	FileMode _mode;

	File(const std::string& rawPath, FileMode mode);
public:
	virtual ~File();

	/**
	 * @brief Only needed after you have called close(). Otherwise the file is
	 * automatically opened in the given @c FileMode
	 * @return @c false if the file could not get opened, or it is still opened,
	 * @c true otherwise
	 */
	bool open(FileMode mode);
	void close();
	int read(void *buf, size_t size, size_t maxnum);
	long tell() const;
	long seek(long offset, int seekType) const;

	/**
	 * @return The FileMode the file was opened with
	 */
	FileMode mode() const;

	bool exists() const;
	bool validHandle() const;
	/**
	 * @return -1 on error, otherwise the length of the file
	 */
	long length() const;
	/**
	 * @return The extension of the file - or en ampty string
	 * if no extension was found
	 */
	std::string extension() const;
	/**
	 * @return The path of the file, without the name - or an
	 * empty string if no path component was found
	 */
	std::string path() const;
	/**
	 * @return Just the base file name component part - without
	 * path and extension
	 */
	std::string fileName() const;
	/**
	 * @return The full raw path of the file
	 */
	const std::string& name() const;

	SDL_RWops* createRWops(FileMode mode) const;
	long write(const unsigned char *buf, size_t len) const;
	/**
	 * @brief Reads the content of the file into a buffer. The buffer is allocated inside this function.
	 * The returned memory is owned by the caller.
	 * @return The amount of read bytes. A value < 0 indicates an error. 0 means empty file.
	 */
	int read(void **buffer);
	int read(void *buffer, int n);
	std::string load();
};

inline FileMode File::mode() const {
	return _mode;
}

typedef std::shared_ptr<File> FilePtr;

}
