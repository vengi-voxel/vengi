/**
 * @file
 */

#include "Emscripten.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"

#include <emscripten/fetch.h>

namespace http {

#if 0
static void fetchErrorCallback(emscripten_fetch_t *fetch) {
	io::WriteStream *stream = static_cast<io::WriteStream *>(fetch->userData);
	emscripten_fetch_close(fetch);
}

static void fetchProgressCallback(emscripten_fetch_t *fetch) {
	io::WriteStream *stream = static_cast<io::WriteStream *>(fetch->userData);
}

static void fetchSuccessCallback(emscripten_fetch_t *fetch) {
	io::WriteStream *stream = static_cast<io::WriteStream *>(fetch->userData);
	emscripten_fetch_close(fetch);
}
#endif

bool http_request(io::WriteStream &stream, int *statusCode, core::StringMap<core::String> *outheaders, RequestContext &ctx) {
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	// EMSCRIPTEN_FETCH_STREAM_DATA
	// If passed, the intermediate streamed bytes will be passed in to the
	// onprogress() handler. If not specified, the onprogress() handler will still
	// be called, but without data bytes.  Note: Firefox only as it depends on
	// 'moz-chunked-arraybuffer'.
	//
	// EMSCRIPTEN_FETCH_SYNCHRONOUS
	// If specified, emscripten_fetch() will synchronously run to completion before
	// returning.  The callback handlers will be called from within
	// emscripten_fetch() while the operation is in progress.
	//
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
	// attr.attributes = EMSCRIPTEN_FETCH_STREAM_DATA | EMSCRIPTEN_FETCH_SYNCHRONOUS;
	const char *method = ctx._type == RequestType::GET ? "GET" : "POST";
	core::string::strncpyz(method, strlen(method), attr.requestMethod, sizeof(attr.requestMethod));

	core::DynamicArray<const char *> headers;
	headers.reserve((ctx._headers.size() + (ctx._userAgent.empty() ? 0 : 2)) * 2 + 1);
	if (!ctx._userAgent.empty()) {
		headers.push_back("User-Agent");
		headers.push_back(ctx._userAgent.c_str());
	}
	for (const auto &entry : ctx._headers) {
		headers.push_back(entry->first.c_str());
		headers.push_back(entry->second.c_str());
	}
	headers.push_back(nullptr);
	attr.requestHeaders = headers.data();
	attr.timeoutMSecs = ctx._timeoutSecond * 1000;
	// attr.userData = &stream;
	// attr.onerror = fetchErrorCallback;
	// attr.onprogress = fetchProgressCallback;
	// attr.onsuccess = fetchSuccessCallback;

	if (!ctx._body.empty()) {
		attr.requestData = ctx._body.c_str();
		attr.requestDataSize = ctx._body.size();
	} else {
		attr.requestData = nullptr;
		attr.requestDataSize = 0;
	}

	emscripten_fetch_t *fetch = emscripten_fetch(&attr, ctx._url.c_str());
	if (fetch == nullptr) {
		Log::error("Http request for '%s' failed", ctx._url.c_str());
		return false;
	}
	if (statusCode) {
		*statusCode = (int)fetch->status;
	}
	if (outheaders) {
		size_t headersLengthBytes = emscripten_fetch_get_response_headers_length(fetch) + 1;
		char *headerString = new char[headersLengthBytes];
		emscripten_fetch_get_response_headers(fetch, headerString, headersLengthBytes);
		char **responseHeaders = emscripten_fetch_unpack_response_headers(headerString);
		delete[] headerString;

		for (int numHeaders = 0; responseHeaders[numHeaders * 2]; ++numHeaders) {
			if (responseHeaders[(numHeaders * 2) + 1] == nullptr) {
				break;
			}

			core::String key = (responseHeaders[numHeaders * 2]) ? responseHeaders[numHeaders * 2] : "";
			core::String value = (responseHeaders[(numHeaders * 2) + 1]) ? responseHeaders[(numHeaders * 2) + 1] : "";
			outheaders->put(key, value);
		}

		emscripten_fetch_free_unpacked_response_headers(responseHeaders);
	}
	Log::debug("Got status code %i for %s", (int)fetch->status, ctx._url.c_str());
	if (stream.write(fetch->data, fetch->numBytes) == -1) {
		Log::error("Failed to write response with %i bytes for url %s", (int)fetch->numBytes, ctx._url.c_str());
		emscripten_fetch_close(fetch);
		return false;
	}
	emscripten_fetch_close(fetch);
	return true;
}

} // namespace http
