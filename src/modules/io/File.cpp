/**
 * @file
 */

#include "File.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include <SDL.h>

namespace io {

void normalizePath(core::String& str) {
	core::string::replaceAllChars(str, '\\', '/');
#ifndef __WINDOWS__
	if (str.size() >= 3 && str[0] != '\0' && core::string::isAlpha(str[0]) && str[1] == ':' && (str[2] == '\\' || str[2] == '/')) {
		str.erase(0, 2);
	}
#endif
}

File::File(const core::String& rawPath, FileMode mode) :
		IOResource(), _rawPath(rawPath), _mode(mode) {
	normalizePath(_rawPath);
	_file = createRWops(mode);
}

File::~File() {
	close();
}

bool File::validHandle() const {
	return _file != nullptr;
}

bool File::isAnyOf(const io::FormatDescription* desc) const {
	const core::String& ext = extension();
	while (desc->valid()) {
		if (desc->matchesExtension(ext)) {
			// TODO: isA check
			return true;
		}
		++desc;
	}
	return false;
}

bool File::exists() const {
	if (_mode == FileMode::Read || _mode == FileMode::SysRead) {
		return _file != nullptr;
	}

	// try to open in read mode
	SDL_RWops* ops = createRWops(FileMode::SysRead);
	if (ops != nullptr) {
		SDL_RWclose(ops);
		return true;
	}
	return false;
}

const core::String& File::name() const {
	return _rawPath;
}

core::String File::load() {
	char *buf = nullptr;
	const int len = read((void **) &buf);
	if (buf == nullptr || len <= 0) {
		delete[] buf;
		return "";
	}
	core::String f(buf, len);
	delete[] buf;
	return f;
}

SDL_RWops* File::createRWops(FileMode mode) const {
	if (_rawPath.empty()) {
		Log::debug("Can't open file - no path given");
		return nullptr;
	}
	const char *fmode = "rb";
	if (mode == FileMode::Write || mode == FileMode::SysWrite) {
		fmode = "wb";
	}
	SDL_RWops *rwops = SDL_RWFromFile(_rawPath.c_str(), fmode);
	if (rwops == nullptr) {
		Log::debug("Can't open file %s: %s", _rawPath.c_str(), SDL_GetError());
	}
	return rwops;
}

long File::write(io::ReadStream &stream) const {
	if (_file == nullptr) {
		Log::debug("Invalid file handle - can write stream (path: %s)", _rawPath.c_str());
		return -1L;
	}
	if (_mode != FileMode::Write && _mode != FileMode::SysWrite) {
		Log::debug("Invalid file mode given - can write stream (path: %s)", _rawPath.c_str());
		return -1L;
	}
	char buf[4096 * 10];
	long l = 0;
	while (!stream.eos()) {
		const int len = stream.read(buf, sizeof(buf));
		if (len == -1) {
			return -1L;
		}
		const size_t written = SDL_RWwrite(_file, buf, 1, len);
		if (written == 0) {
			Log::debug("Error writing file - can write buffer of length %i (path: %s)", (int)len, _rawPath.c_str());
			return -1L;
		}
		l += len;
	}

	Log::debug("%i bytes were written into path %s", (int)l, _rawPath.c_str());
	return l;
}

long File::write(const unsigned char *buf, size_t len) const {
	if (_file == nullptr) {
		Log::debug("Invalid file handle - can write buffer of length %i (path: %s)",
				(int)len, _rawPath.c_str());
		return -1L;
	}
	if (_mode != FileMode::Write && _mode != FileMode::SysWrite) {
		Log::debug("Invalid file mode given - can write buffer of length %i (path: %s)",
				(int)len, _rawPath.c_str());
		return -1L;
	}

	int remaining = (int)len;
	while (remaining > 0) {
		const size_t written = SDL_RWwrite(_file, buf, 1, remaining);
		if (written == 0) {
			Log::debug("Error writing file - can write buffer of length %i (remaining: %i) (path: %s)",
					(int)len, remaining, _rawPath.c_str());
			return -1L;
		}

		remaining -= (int)written;
		buf += written;
	}

	Log::debug("%i bytes were written into path %s", (int)len, _rawPath.c_str());
	return (long)len;
}

core::String File::path() const {
	return core::string::extractPath(name());
}

core::String File::fileName() const {
	return core::string::extractFilename(name());
}

core::String File::extension() const {
	const char *ext = SDL_strrchr(name().c_str(), '.');
	if (ext == nullptr) {
		return "";
	}
	++ext;
	return core::String(ext);
}

long File::length() const {
	if (!exists()) {
		return -1l;
	}

	const long pos = tell();
	seek(0, RW_SEEK_END);
	const long end = tell();
	seek(pos, RW_SEEK_SET);
	return end;
}

int File::read(void **buffer) {
	*buffer = nullptr;
	const long len = length();
	if (len <= 0) {
		return len;
	}
	*buffer = new uint8_t[len];
	return read(*buffer, len);
}

int File::read(void *buffer, int n) {
	const size_t blockSize = 0x10000;
	uint8_t *buf;
	size_t remaining, len;

	len = remaining = n;
	buf = (uint8_t *) buffer;

	seek(0, RW_SEEK_SET);

	while (remaining != 0u) {
		size_t block = remaining;
		if (block > blockSize) {
			block = blockSize;
		}
		const int readAmount = read(buf, 1, block);

		/* end of file reached */
		if (readAmount == 0) {
			return int(len - remaining + readAmount);
		} else if (readAmount == -1) {
			Log::debug("Read error while reading %s", _rawPath.c_str());
			return -1;
		}

		/* do some progress bar thing here... */
		remaining -= readAmount;
		buf += readAmount;
	}
	Log::debug("Read %i bytes from %s", (int)len, _rawPath.c_str());
	return (int)len;
}

int File::read(void *buf, size_t size, size_t maxnum) {
	if (_mode != FileMode::Read && _mode != FileMode::SysRead) {
		_state = IOSTATE_FAILED;
		Log::debug("File %s is not opened in read mode", _rawPath.c_str());
		return -1;
	}
	const int n = (int)SDL_RWread(_file, buf, size, maxnum);
	if (n == 0) {
		_state = IOSTATE_LOADED;
		Log::trace("File %s: read successful", _rawPath.c_str());
	} else if (n == -1) {
		_state = IOSTATE_FAILED;
		Log::trace("File %s: read failed", _rawPath.c_str());
	} else {
		Log::trace("File %s: read %i bytes", _rawPath.c_str(), n);
	}

	return n;
}

bool File::flush() {
	if (_file != nullptr) {
		SDL_RWclose(_file);
		_file = createRWops(_mode);
		return _file != nullptr;
	}
	return false;
}

void File::close() {
	if (_file != nullptr) {
		SDL_RWclose(_file);
		_file = nullptr;
	}
}

bool File::open(FileMode mode) {
	if (_file != nullptr) {
		Log::debug("File %s is already open", _rawPath.c_str());
		return false;
	}
	_mode = mode;
	_file = createRWops(mode);
	return _file != nullptr;
}

long File::tell() const {
	if (_file != nullptr) {
		return SDL_RWtell(_file);
	}
	return -1L;
}

long File::seek(long offset, int seekType) const {
	if (_file != nullptr) {
		return SDL_RWseek(_file, offset, seekType);
	}
	return -1L;
}

}
