/**
 * @file
 */

#include "Emscripten.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"

#include <emscripten/fetch.h>
#include <emscripten/threading.h>

namespace http {

struct RequestResponse {
	int statusCode;
	io::WriteStream *stream = nullptr;
	core::StringMap<core::String> *outheaders = nullptr;
};

static void fetchErrorCallback(emscripten_fetch_t *fetch) {
	RequestResponse *ctx = static_cast<RequestResponse *>(fetch->userData);
	ctx->statusCode = fetch->status;
	Log::debug("Error %i", fetch->status);
	emscripten_fetch_close(fetch);
}

static void fetchSuccessCallback(emscripten_fetch_t *fetch) {
	RequestResponse *ctx = static_cast<RequestResponse *>(fetch->userData);
	ctx->statusCode = fetch->status;
	ctx->stream->write(fetch->data, fetch->numBytes);
	Log::debug("Got %i bytes with status: %i", (int)fetch->numBytes, ctx->statusCode);
	if (ctx->outheaders) {
		size_t headersLengthBytes = emscripten_fetch_get_response_headers_length(fetch) + 1;
		char *headerString = new char[headersLengthBytes];
		emscripten_fetch_get_response_headers(fetch, headerString, headersLengthBytes);
		char **responseHeaders = emscripten_fetch_unpack_response_headers(headerString);

		for (int numHeaders = 0; responseHeaders[numHeaders * 2]; ++numHeaders) {
			if (responseHeaders[(numHeaders * 2) + 1] == nullptr) {
				break;
			}

			core::String key = (responseHeaders[numHeaders * 2]) ? responseHeaders[numHeaders * 2] : "";
			core::String value = (responseHeaders[(numHeaders * 2) + 1]) ? responseHeaders[(numHeaders * 2) + 1] : "";
			ctx->outheaders->put(key, value);
		}

		emscripten_fetch_free_unpacked_response_headers(responseHeaders);
		delete[] headerString;
	}
	emscripten_fetch_close(fetch);
}

static bool http_request_sync(RequestResponse &userdata, RequestContext &ctx) {
	Log::debug("http_request_sync");
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

	core_assert(!emscripten_is_main_browser_thread());
	const char *method = ctx._type == RequestType::GET ? "GET" : "POST";
	core::string::strncpyz(method, strlen(method), attr.requestMethod, sizeof(attr.requestMethod));

	Log::debug("Requesting %s via %s", ctx._url.c_str(), attr.requestMethod);
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
	attr.requestHeaders = nullptr; // headers.data();
	attr.timeoutMSecs = ctx._timeoutSecond * 1000;
	Log::debug("timeoutMSecs %i", attr.timeoutMSecs);

	attr.userData = &userdata;
	attr.onerror = fetchErrorCallback;
	attr.onsuccess = fetchSuccessCallback;

	if (!ctx._body.empty()) {
		attr.requestData = ctx._body.c_str();
		attr.requestDataSize = ctx._body.size();
	} else {
		attr.requestData = nullptr;
		attr.requestDataSize = 0;
	}

	emscripten_fetch_t *fetch = emscripten_fetch(&attr, ctx._url.c_str());
	if (!fetch) {
		Log::error("Failed to fetch %s", ctx._url.c_str());
		return false;
	}
	return true;
}

bool http_request(io::WriteStream &stream, int *statusCode, core::StringMap<core::String> *outheaders,
				  RequestContext &ctx) {
	if (emscripten_is_main_browser_thread()) {
		Log::error("http_request called from main thread - this doesn't work for emscripten");
		// https://github.com/emscripten-core/emscripten/issues/8070
		return false;
	}

	RequestResponse userdata;
	userdata.stream = &stream;
	userdata.outheaders = outheaders;
	const bool state = http_request_sync(userdata, ctx);
	if (statusCode) {
		*statusCode = userdata.statusCode;
	}

	Log::debug("Got status code %i for %s", userdata.statusCode, ctx._url.c_str());
	return state;
}

} // namespace http
