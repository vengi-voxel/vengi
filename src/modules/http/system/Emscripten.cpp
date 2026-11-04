/**
 * @file
 */

#include "Emscripten.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include <emscripten/threading.h>

namespace http {

void configureFetch(emscripten_fetch_attr_t &attr, const RequestContext &ctx, core::Buffer<const char *> &headers) {
	emscripten_fetch_attr_init(&attr);
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_REPLACE;
	const char *method = ctx._type == RequestType::GET ? "GET" : (ctx._type == RequestType::PATCH ? "PATCH" : "POST");
	core::string::strncpyz(method, strlen(method), attr.requestMethod, sizeof(attr.requestMethod));
	// Browsers own User-Agent and Cookie. Custom headers remain subject to CORS.
	for (const auto &entry : ctx._headers) {
		headers.push_back(entry->first.c_str());
		headers.push_back(entry->second.c_str());
	}
	headers.push_back(nullptr);
	attr.requestHeaders = headers.data();
	attr.timeoutMSecs = ctx._timeoutSecond > 0 ? ctx._timeoutSecond * 1000 : 0;
	attr.requestData = ctx._body.empty() ? nullptr : (const char *)ctx._body.data();
	attr.requestDataSize = ctx._body.size();
}

void readFetchHeaders(emscripten_fetch_t *fetch, Headers &headers) {
	const size_t length = emscripten_fetch_get_response_headers_length(fetch) + 1;
	core::Buffer<char> text;
	text.resize(length);
	text[length - 1] = '\0';
	emscripten_fetch_get_response_headers(fetch, text.data(), length);
	char **unpacked = emscripten_fetch_unpack_response_headers(text.data());
	if (!unpacked) {
		return;
	}
	for (int i = 0; unpacked[i] && unpacked[i + 1]; i += 2) {
		headers.put(unpacked[i], core::string::trim(unpacked[i + 1]));
	}
	emscripten_fetch_free_unpacked_response_headers(unpacked);
}

bool http_request(io::WriteStream &stream, int *statusCode, Headers *outheaders, RequestContext &ctx) {
	if (emscripten_is_main_browser_thread()) {
		Log::error("Synchronous HTTP requires a worker thread; use http::Client on the application thread");
		return false;
	}
	emscripten_fetch_attr_t attr;
	core::Buffer<const char *> headers;
	configureFetch(attr, ctx, headers);
	attr.attributes |= EMSCRIPTEN_FETCH_SYNCHRONOUS;
	emscripten_fetch_t *fetch = emscripten_fetch(&attr, ctx._url.c_str());
	if (!fetch) {
		return false;
	}
	if (statusCode) {
		*statusCode = fetch->status;
	}
	if (outheaders) {
		readFetchHeaders(fetch, *outheaders);
	}
	const bool success =
		fetch->status > 0 && fetch->status < 600 &&
		(fetch->numBytes == 0 || stream.write(fetch->data, fetch->numBytes) == (int64_t)fetch->numBytes);
	emscripten_fetch_close(fetch);
	return success;
}

} // namespace http
