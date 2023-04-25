/**
 * @file
 */

#include "FileStream.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "io/File.h"
#include <SDL_endian.h>
#include <SDL_rwops.h>
#include <stdio.h>
#include <stdarg.h>

namespace io {

FileStream::FileStream(const FilePtr &file) : _file(file) {
	if (_file) {
		_rwops = _file->_file;
		if (_rwops) {
			_size = SDL_RWsize(_rwops);
		} else {
			_size = 0;
		}
	} else {
		_rwops = nullptr;
		_size = 0;
	}
}

FileStream::~FileStream() {
}

bool FileStream::valid() const {
	return _file && _file->validHandle();
}

bool FileStream::flush() {
	if (_rwops == nullptr) {
		return false;
	}
	return _file->flush();
}

int FileStream::write(const void *buf, size_t size) {
	if (_rwops == nullptr) {
		return -1;
	}
	if (size == 0) {
		return 0;
	}
	const int64_t written = (int64_t)SDL_RWwrite(_rwops, buf, 1, size);
	if (written != (int64_t)size) {
		Log::debug("File write error: %s", SDL_GetError());
		return -1;
	}
	_pos = SDL_RWtell(_rwops);
	_size = core_max(_size, _pos);
	return (int)written;
}

int FileStream::read(void *dataPtr, size_t dataSize) {
	if (_rwops == nullptr) {
		return -1;
	}
	uint8_t *b = (uint8_t*)dataPtr;
	size_t completeBytesRead = 0;
	size_t bytesRead = 1;
	while (completeBytesRead < dataSize && bytesRead != 0) {
		bytesRead = SDL_RWread(_rwops, b, 1, (dataSize - completeBytesRead));
		b += bytesRead;
		completeBytesRead += bytesRead;
	}
	_pos = SDL_RWtell(_rwops);
	return (int)completeBytesRead;
}

int64_t FileStream::seek(int64_t position, int whence) {
	if (_rwops == nullptr) {
		return -1;
	}
	int64_t p = SDL_RWseek(_rwops, position, whence);
	_pos = SDL_RWtell(_rwops);
	if (p == -1) {
		return -1;
	}
	return 0;
}

} // namespace io
