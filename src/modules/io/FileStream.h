/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "io/Stream.h"
#include <SDL_rwops.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

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
class FileStream : public SeekableReadStream, public SeekableWriteStream {
private:
	mutable SDL_RWops *_rwops;
	FilePtr _file;
	int64_t _size = 0;
	int64_t _pos = 0;
public:
	FileStream(const FilePtr &file);
	virtual ~FileStream();

	bool valid() const;
	int64_t size() const override;
	int64_t pos() const override;
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
