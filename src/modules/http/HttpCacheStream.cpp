/**
 * @file
 */

#include "HttpCacheStream.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Http.h"
#include "http/Request.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

namespace http {

HttpCacheStream::HttpCacheStream(const io::ArchivePtr &archive, const core::String &file, const core::String &url) {
	initGet(archive, file, url);
}

HttpCacheStream::HttpCacheStream(const io::ArchivePtr &archive, const core::String &file, const core::String &url,
								 const core::String &postBody, const core::String &contentType) {
	initPost(archive, file, url, postBody, contentType);
}

void HttpCacheStream::initGet(const io::ArchivePtr &archive, const core::String &file, const core::String &url) {
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
	http::Headers outheaders;
	if (http::download(url, bufStream, &statusCode, &outheaders)) {
		// TODO: HTTP: handle these headers https://www.ietf.org/archive/id/draft-polli-ratelimit-headers-02.html
		// x-ratelimit-remaining "<number>"
		// x-ratelimit-limit "<number>"
		// x-ratelimit-used "<number>"
		// x-ratelimit-reset "<timestamp utc>"
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
		return core::String::Empty;
	}
	core::String str;
	stream.readString((int)stream.size(), str);
	return str;
}

core::String HttpCacheStream::stringPost(const io::ArchivePtr &archive, const core::String &file,
										 const core::String &url, const core::String &postBody,
										 const core::String &contentType) {
	HttpCacheStream stream(archive, file, url, postBody, contentType);
	if (!stream.valid()) {
		return core::String::Empty;
	}
	core::String str;
	stream.readString((int)stream.size(), str);
	return str;
}

void HttpCacheStream::initPost(const io::ArchivePtr &archive, const core::String &file, const core::String &url,
							   const core::String &postBody, const core::String &contentType) {
	if (archive->exists(file)) {
		Log::debug("Use cached file at %s for POST %s", file.c_str(), url.c_str());
		_readStream = archive->readStream(file);
		core_assert(_readStream != nullptr);
		return;
	}
	Log::debug("try to POST to %s (cache: %s)", url.c_str(), file.c_str());
	io::BufferedReadWriteStream bufStream(1024 * 1024);

	http::Request request(url, http::RequestType::POST);
	request.setBody(postBody);
	request.addHeader("Content-Type", contentType);
	request.setTimeoutSecond(180);
	request.setConnectTimeoutSecond(30);

	int statusCode = 0;
	if (request.execute(bufStream, &statusCode)) {
		if (http::isValidStatusCode(statusCode)) {
			write(archive, file, bufStream);
		} else if (statusCode == 429) {
			Log::warn("Too many requests, retrying in 5 seconds... POST %s (%s)", url.c_str(), file.c_str());
			app::App::getInstance()->wait(5000);
			bufStream.seek(0);
			http::Request retryRequest(url, http::RequestType::POST);
			retryRequest.setBody(postBody);
			retryRequest.addHeader("Content-Type", contentType);
			retryRequest.setTimeoutSecond(180);
			retryRequest.setConnectTimeoutSecond(30);
			if (retryRequest.execute(bufStream, &statusCode)) {
				if (http::isValidStatusCode(statusCode)) {
					write(archive, file, bufStream);
				}
			}
		} else {
			Log::warn("Failed to POST %s (%s) - HTTP %d", url.c_str(), file.c_str(), statusCode);
		}
	} else {
		Log::warn("Failed to POST %s (%s)", url.c_str(), file.c_str());
	}
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
