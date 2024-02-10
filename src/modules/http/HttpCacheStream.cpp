/**
 * @file
 */

#include "HttpCacheStream.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Http.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"

namespace http {

HttpCacheStream::HttpCacheStream(const io::FilesystemPtr &fs, const core::String &file, const core::String &url) : _file(file), _url(url) {
	if (core::string::startsWith(url, "file://")) {
		_fileStream = new io::FileStream(fs->open(url.substr(7), io::FileMode::Read));
		return;
	}
	if (!fs->exists(file)) {
		io::BufferedReadWriteStream bufStream(1024 * 1024);
		int statusCode = 0;
		if (http::download(url, bufStream, &statusCode)) {
			if (http::isValidStatusCode(statusCode)) {
				bufStream.seek(0);
				if (fs->write(file, bufStream)) {
					_fileStream = new io::FileStream(fs->open(file, io::FileMode::Read));
					_newInCache = true;
				}
			}
		}
	} else {
		Log::debug("Use cached file at %s", file.c_str());
		_fileStream = new io::FileStream(fs->open(file, io::FileMode::Read));
	}
}

HttpCacheStream::~HttpCacheStream() {
	delete _fileStream;
}

bool HttpCacheStream::valid() const {
	if (_fileStream == nullptr) {
		return false;
	}
	return _fileStream->valid();
}

int HttpCacheStream::read(void *dataPtr, size_t dataSize) {
	if (!valid()) {
		return -1;
	}
	return _fileStream->read(dataPtr, dataSize);
}

int64_t HttpCacheStream::seek(int64_t position, int whence) {
	if (!valid()) {
		return -1;
	}
	return _fileStream->seek(position, whence);
}

int64_t HttpCacheStream::size() const {
	if (!valid()) {
		return -1;
	}
	return _fileStream->size();
}

int64_t HttpCacheStream::pos() const {
	if (!valid()) {
		return 0;
	}
	return _fileStream->pos();
}

} // namespace http
