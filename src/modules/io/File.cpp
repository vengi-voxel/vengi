/**
 * @file
 */

#include "File.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/concurrent/Lock.h"
#include "io/FormatDescription.h"
#include "io/system/System.h"
#ifdef __EMSCRIPTEN__
#include "system/emscripten_browser_file.h"
#endif
#include <SDL_version.h>
#include <SDL_error.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#include <SDL_iostream.h>
#define SDL_RWclose SDL_CloseIO
#define RW_SEEK_CUR SDL_IO_SEEK_CUR
#define RW_SEEK_END SDL_IO_SEEK_END
#define RW_SEEK_SET SDL_IO_SEEK_SET
#define SDL_RWsize SDL_GetIOSize
#define SDL_RWtell SDL_TellIO
#define SDL_RWseek(ctx, pos, whence) SDL_SeekIO(ctx, pos, (SDL_IOWhence)whence)
#define SDL_RWread(ctx, ptr, size, maxnum) SDL_ReadIO(ctx, ptr, maxnum)
#define SDL_RWwrite(ctx, ptr, size, num) SDL_WriteIO(ctx, ptr, num)
#define SDL_RWFromFile SDL_IOFromFile
#else
#include <SDL_rwops.h>
#endif

namespace io {

namespace priv {
// windows can only open a file once - we are tracking opened files here to get
// error logs on other systems to be able to debug and fix the issues that otherwise
// would only be visible to windows users
core::StringMap<FileMode> _openedFiles;
core_trace_mutex(core::Lock, _openedFileLock, "openedFileLock");

void trackOpenedFile(const core::String &path, FileMode mode) {
	core::ScopedLock lock(_openedFileLock);
	core::String absPath = fs_realpath(path.c_str());
	normalizePath(absPath);
	if (absPath.empty()) {
		Log::debug("Failed to track opened file %s", path.c_str());
		return;
	}
	const bool alreadyOpened = _openedFiles.hasKey(absPath);
	if (alreadyOpened) {
		FileMode openedMode = FileMode::Max;
		core_assert_always(_openedFiles.get(absPath, openedMode));
		core_assert_msg(
			!alreadyOpened,
			"File %s is already opened (opened mode %s, new mode %s) - this will produce problems on windows",
			path.c_str(), FileModeStr[(int)openedMode], FileModeStr[(int)mode]);

		if (openedMode == FileMode::Max) {
			Log::error("File %s is already opened (new mode %s)", path.c_str(), FileModeStr[(int)mode]);
			return;
		}
		Log::error("File %s is already opened (opened mode %s, new mode %s)", path.c_str(),
				   FileModeStr[(int)openedMode], FileModeStr[(int)mode]);
		return;
	}
	Log::debug("open file: %s (mode %s)", absPath.c_str(), FileModeStr[(int)mode]);
	_openedFiles.put(absPath, mode);
}

void untrackOpenedFile(const core::String &path, FileMode mode) {
	core::ScopedLock lock(_openedFileLock);
	core::String absPath = fs_realpath(path.c_str());
	normalizePath(absPath);
	if (absPath.empty()) {
		Log::debug("Failed to track opened file %s", path.c_str());
		return;
	}
	if (!_openedFiles.hasKey(absPath)) {
		Log::debug("File %s is not tracked as being opened", path.c_str());
		return;
	}
	Log::debug("close file: %s (mode %s)", absPath.c_str(), FileModeStr[(int)mode]);
	_openedFiles.remove(absPath);
}

}

core::String normalizePath(const core::String& str) {
	core::String s = str;
	normalizePath(s);
	return s;
}

void normalizePath(core::String& str) {
	core::string::replaceAllChars(str, '\\', '/');
#if !defined(_WIN32) && !defined(__CYGWIN__)
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

File::File(core::String&& rawPath, FileMode mode) :
		IOResource(), _rawPath(core::move(rawPath)), _mode(mode) {
	normalizePath(_rawPath);
	_file = createRWops(mode);
}

File::~File() {
	close();
}

bool File::validHandle() const {
	return _file != nullptr;
}

bool File::exists() const {
	if (_mode == FileMode::Read || _mode == FileMode::SysRead) {
		return _file != nullptr;
	}

	return fs_exists(_rawPath.c_str());
}

const core::String& File::name() const {
	return _rawPath;
}

core::String File::load() {
	char *buf = nullptr;
	const int len = read((void **) &buf);
	if (buf == nullptr || len <= 0) {
		delete[] buf;
		return core::String::Empty;
	}
	core::String f(buf, len);
	delete[] buf;
	return f;
}

void File::error(const char *msg, ...) const {
	va_list ap;
	constexpr size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(text, bufSize, msg, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	_error = text;
	Log::debug("path: '%s' (mode: %i): %s", _rawPath.c_str(), (int)_mode, _error.c_str());
}

void File::closeRWops(SDL_RWops *handle) const {
	if (handle != nullptr) {
		SDL_RWclose(handle);
		priv::untrackOpenedFile(_rawPath, _mode);
	}
}

SDL_RWops* File::createRWops(FileMode mode) const {
	if (_rawPath.empty()) {
		error("Can't open file - no path given");
		return nullptr;
	}
	const char *fmode = "rb";
	if (mode == FileMode::Write || mode == FileMode::SysWrite) {
		fmode = "wb";
	} else if (mode == FileMode::Append) {
		fmode = "ab";
	}
	SDL_RWops *rwops = SDL_RWFromFile(_rawPath.c_str(), fmode);
	if (rwops == nullptr) {
		error("%s", SDL_GetError());
	} else {
		priv::trackOpenedFile(_rawPath, mode);
	}
	return rwops;
}

long File::write(io::ReadStream &stream) const {
	if (_file == nullptr) {
		error("Invalid file handle - can't write to file");
		return -1L;
	}
	if (_mode != FileMode::Write && _mode != FileMode::SysWrite) {
		error("Invalid file mode given - can't write to file");
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
			error("Error writing file - failed to write buffer of length %i", (int)len);
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

core::String File::dir() const {
	return core::string::extractDir(name());
}

core::String File::fileName() const {
	return core::string::extractFilename(name());
}

core::String File::extension() const {
	const char *ext = SDL_strrchr(name().c_str(), '.');
	if (ext == nullptr) {
		return core::String::Empty;
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
		if (_mode != FileMode::Read && _mode != FileMode::SysRead) {
			_state = IOSTATE_FAILED;
			Log::debug("File %s is not opened in read mode", _rawPath.c_str());
			return -1;
		}
		const int readAmount = (int)SDL_RWread(_file, buf, 1, block);
		if (readAmount == 0) {
			_state = IOSTATE_LOADED;
			Log::trace("File %s: read successful", _rawPath.c_str());
			return int(len - remaining + readAmount);
		} else if (readAmount == -1) {
			_state = IOSTATE_FAILED;
			Log::trace("File %s: read failed", _rawPath.c_str());
			return -1;
		} else {
			Log::trace("File %s: read %i bytes", _rawPath.c_str(), n);
		}

		/* do some progress bar thing here... */
		remaining -= readAmount;
		buf += readAmount;
	}
	Log::debug("Read %i bytes from %s", (int)len, _rawPath.c_str());
	return (int)len;
}

bool File::flush() {
	if (_file != nullptr) {
		closeRWops(_file);

		if (_mode == FileMode::Write || _mode == FileMode::SysWrite) {
			_mode = FileMode::Append;
		}
		_file = createRWops(_mode);
		return _file != nullptr;
	}
	return false;
}

void File::close() {
	if (_file != nullptr) {
		closeRWops(_file);
		_file = nullptr;
#ifdef __EMSCRIPTEN__
		if (_mode == FileMode::SysWrite) {
			_file = createRWops(FileMode::SysRead);
			_mode = FileMode::SysRead;
			if (_file == nullptr) {
				Log::error("Failed to download file %s", _rawPath.c_str());
			} else {
				uint8_t *buf = nullptr;
				const int len = read((void **)&buf);
				if (len > 0) {
					emscripten_browser_file::download(_rawPath.c_str(), "application/octet-stream", buf, (size_t)len);
				}
				delete[] buf;
				closeRWops(_file);
				_file = nullptr;
			}
			_mode = FileMode::SysWrite;
		}
#endif
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
