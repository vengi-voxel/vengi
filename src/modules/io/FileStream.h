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
 * @brief Little endian file stream
 */
class FileStream : public ReadStream, public WriteStream {
private:
	mutable SDL_RWops *_rwops;

public:
	FileStream(File *file);
	FileStream(const FilePtr &file) : FileStream(file.get()) {
	}
	FileStream(SDL_RWops *rwops);
	virtual ~FileStream();

	int read(void *dataPtr, size_t dataSize) override;
	int write(const void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
};

} // namespace io
