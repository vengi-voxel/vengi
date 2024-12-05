/**
 * @file
 */

#include "HttpCacheStream.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Http.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

namespace http {

HttpCacheStream::HttpCacheStream(const io::ArchivePtr &archive, const core::String &file, const core::String &url) {
	if (core::string::startsWith(url, "file://")) {
		_readStream = archive->readStream(url.substr(7));
		return;
	}
	if (archive->exists(file)) {
		Log::debug("Use cached file at %s for %s", file.c_str(), url.c_str());
		_readStream = archive->readStream(file);
		core_assert(_readStream != nullptr);
		return;
	}
	Log::debug("try to download %s from %s", file.c_str(), url.c_str());
	io::BufferedReadWriteStream bufStream(1024 * 1024);
	int statusCode = 0;
	if (http::download(url, bufStream, &statusCode)) {
		if (http::isValidStatusCode(statusCode)) {
			write(archive, file, bufStream);
		} else if (statusCode == 429) {
			Log::warn("Too many requests, retrying in 5 seconds... %s (%s)", url.c_str(), file.c_str());
			app::App::getInstance()->wait(5000);
			if (http::download(url, bufStream, &statusCode)) {
				if (http::isValidStatusCode(statusCode)) {
					write(archive, file, bufStream);
				}
			}
		} else {
			Log::warn("Failed to download %s (%s)", url.c_str(), file.c_str());
		}
	} else {
		Log::warn("Failed to download %s (%s)", url.c_str(), file.c_str());
	}
}

HttpCacheStream::~HttpCacheStream() {
	delete _readStream;
}

core::String HttpCacheStream::string(const io::ArchivePtr &archive, const core::String &file, const core::String &url) {
	HttpCacheStream stream(archive, file, url);
	if (!stream.valid()) {
		return "";
	}
	core::String str;
	stream.readString((int)stream.size(), str);
	return str;
}

void HttpCacheStream::close() {
	delete _readStream;
	_readStream = nullptr;
}

void HttpCacheStream::write(const io::ArchivePtr &archive, const core::String &file,
							io::BufferedReadWriteStream &bufStream) {
	bufStream.seek(0);
	if (io::SeekableWriteStream *ws = archive->writeStream(file)) {
		ws->write(bufStream.getBuffer(), bufStream.size());
		delete ws;
		_readStream = archive->readStream(file);
		_newInCache = true;
		Log::debug("Wrote %s to http cache", file.c_str());
	} else {
		Log::error("Failed to write %s into http cache", file.c_str());
	}
}

bool HttpCacheStream::valid() const {
	if (_readStream == nullptr) {
		return false;
	}
	return true;
}

int HttpCacheStream::read(void *dataPtr, size_t dataSize) {
	if (!valid()) {
		return -1;
	}
	return _readStream->read(dataPtr, dataSize);
}

int64_t HttpCacheStream::seek(int64_t position, int whence) {
	if (!valid()) {
		return -1;
	}
	return _readStream->seek(position, whence);
}

int64_t HttpCacheStream::size() const {
	if (!valid()) {
		return -1;
	}
	return _readStream->size();
}

int64_t HttpCacheStream::pos() const {
	if (!valid()) {
		return 0;
	}
	return _readStream->pos();
}

} // namespace http
