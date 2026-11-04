/**
 * @file
 */

#include "Async.h"
#include "core/concurrent/ThreadPool.h"
#include "engine-config.h"
#if USE_CURL
#include <curl/curl.h>
#endif

namespace http {
namespace detail {
namespace {

core::ThreadPool *workers = nullptr;

} // namespace

bool initAsync() {
#if USE_CURL
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
		return false;
	}
#endif
	workers = new core::ThreadPool(2, "http");
	workers->init();
	return true;
}

void startAsync(const AsyncRequestPtr &request) {
	workers->schedule([request]() {
		if (!request->cancelled) {
			executeRequest(*request.get());
		}
		request->done = true;
	});
}

void cancelAsync(const AsyncRequestPtr &) {
	// Backends can observe RequestContext::_cancelled. Other transports finish
	// their synchronous operation; their callbacks have already been dropped.
}

void shutdownAsync() {
	delete workers;
	workers = nullptr;
#if USE_CURL
	curl_global_cleanup();
#endif
}

} // namespace detail
} // namespace http
