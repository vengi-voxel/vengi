/**
 * @file
 */

#include "FileStream.h"
#include "SDL_rwops.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "io/File.h"
#include <SDL_endian.h>
#include <stdio.h>
#include <stdarg.h>

namespace io {

FileStream::FileStream(File *file) : FileStream(file->_file) {
}

FileStream::FileStream(SDL_RWops *rwops) : _rwops(rwops) {
	core_assert(rwops != nullptr);
	_size = SDL_RWsize(_rwops);
}

FileStream::~FileStream() {
}

int FileStream::write(const void *buf, size_t size) {
	if (size == 0) {
		return 0;
	}
	const int64_t written = (int64_t)SDL_RWwrite(_rwops, buf, 1, size);
	_pos = SDL_RWtell(_rwops);
	_size = core_max(_size, _pos);
	if (written == (int64_t)size) {
		return 0;
	}
	return -1;
}

int FileStream::read(void *dataPtr, size_t dataSize) {
	uint8_t *b = (uint8_t*)dataPtr;
	size_t completeBytesRead = 0;
	size_t bytesRead = 1;
	while (completeBytesRead < dataSize && bytesRead != 0) {
		bytesRead = SDL_RWread(_rwops, b, 1, (dataSize - completeBytesRead));
		b += bytesRead;
		completeBytesRead += bytesRead;
	}
	_pos = SDL_RWtell(_rwops);
	if (completeBytesRead != dataSize) {
		return -1;
	}
	return 0;
}

int64_t FileStream::seek(int64_t position, int whence) {
	int64_t p = SDL_RWseek(_rwops, position, whence);
	_pos = SDL_RWtell(_rwops);
	if (p == -1) {
		return -1;
	}
	return 0;
}

} // namespace io
