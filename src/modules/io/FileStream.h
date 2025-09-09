/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "io/Stream.h"
#include <stddef.h>
#include <stdint.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
struct SDL_IOStream;
#define SDL_RWops SDL_IOStream
#else
struct SDL_RWops;
#endif

namespace io {

class File;
typedef core::SharedPtr<File> FilePtr;

/**
 * @brief File read and write capable stream
 *
 * @note the stream is not flushed automatically. This is either done by calling flush() manually - or when the
 * used file instance is closed.
 * @ingroup IO
 * @see SeekableReadStream
 * @see SeekableWriteStream
 */
class FileStream : public SeekableReadWriteStream {
private:
	mutable SDL_RWops *_rwops = nullptr;
	FilePtr _file;
	int64_t _size = -1;
	int64_t _pos = 0;
public:
	FileStream(const FilePtr &file);
	virtual ~FileStream();

	void close() override;
	bool valid() const;
	int64_t size() const override;
	int64_t pos() const override;
	bool eos() const override;
	int read(void *dataPtr, size_t dataSize) override;
	int write(const void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	/**
	 * @brief Flush the pending stream data into the output stream. This is closing the file
	 * and re-open if afterwards.
	 */
	bool flush() override;
};

inline int64_t FileStream::size() const {
	return _size;
}

inline int64_t FileStream::pos() const {
	return _pos;
}

} // namespace io
