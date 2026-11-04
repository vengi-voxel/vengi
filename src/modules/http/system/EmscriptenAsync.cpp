/**
 * @file
 */

#include "Async.h"
#include "Emscripten.h"

namespace http {
namespace detail {
namespace {

void completed(emscripten_fetch_t *fetch) {
	AsyncRequest *request = (AsyncRequest *)fetch->userData;
	// Closing an in-flight fetch can synchronously invoke its error callback.
	// The cancelling caller owns the close in that case.
	if (request->cancelled) {
		return;
	}
	Response &response = request->response;
	response.statusCode = fetch->status;
	response.success = fetch->status > 0 && fetch->status < 600;
	if (fetch->numBytes > 0 && fetch->data) {
		response.body.append((const uint8_t *)fetch->data, (size_t)fetch->numBytes);
	}
	readFetchHeaders(fetch, response.headers);
	request->done = true;
}

} // namespace

bool initAsync() {
	return true;
}

void startAsync(const AsyncRequestPtr &request) {
	emscripten_fetch_attr_t attr;
	core::Buffer<const char *> headers;
	configureFetch(attr, request->context, headers);
	attr.userData = request.get();
	attr.onsuccess = completed;
	attr.onerror = completed;
	request->transport = emscripten_fetch(&attr, request->context._url.c_str());
	if (!request->transport) {
		request->done = true;
	}
}

void cancelAsync(const AsyncRequestPtr &request) {
	emscripten_fetch_t *fetch = (emscripten_fetch_t *)request->transport;
	request->transport = nullptr;
	if (fetch) {
		emscripten_fetch_close(fetch);
	}
}

void shutdownAsync() {
}

} // namespace detail
} // namespace http
