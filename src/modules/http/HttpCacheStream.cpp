/**
 * @file
 */

#include "HttpCacheStream.h"
#include "app/App.h"
#include "http/Http.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"

namespace http {

HttpCacheStream::HttpCacheStream(const core::String &file, const core::String &url) : _file(file), _url(url) {
	if (!io::filesystem()->exists(file)) {
		io::BufferedReadWriteStream bufStream;
		int statusCode = 0;
		if (http::download(url, bufStream, &statusCode)) {
			if (http::isValidStatusCode(statusCode)) {
				bufStream.seek(0);
				if (io::filesystem()->write(file, bufStream)) {
					_fileStream = new io::FileStream(io::filesystem()->open(file, io::FileMode::Read));
					_newInCache = true;
				}
			}
		}
	} else {
		_fileStream = new io::FileStream(io::filesystem()->open(file, io::FileMode::Read));
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
