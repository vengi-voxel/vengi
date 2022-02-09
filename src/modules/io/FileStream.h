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
 * @ingroup IO
 * @see SeekableReadStream
 * @see SeekableWriteStream
 */
class FileStream : public SeekableReadStream, public SeekableWriteStream {
private:
	mutable SDL_RWops *_rwops;
	int64_t _size = 0;
	int64_t _pos = 0;
public:
	FileStream(File *file);
	// TODO: store the fileptr
	FileStream(const FilePtr &file) : FileStream(file.get()) {
	}
	FileStream(SDL_RWops *rwops);
	virtual ~FileStream();

	int64_t size() const override;
	int64_t pos() const override;
	int read(void *dataPtr, size_t dataSize) override;
	int write(const void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
};

inline int64_t FileStream::size() const {
	return _size;
}

inline int64_t FileStream::pos() const {
	return _pos;
}

} // namespace io
