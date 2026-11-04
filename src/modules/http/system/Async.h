/**
 * @file
 * @brief Internal interface shared by the asynchronous HTTP backends.
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/concurrent/Atomic.h"
#include "http/Client.h"

namespace http {
namespace detail {

struct AsyncRequest {
	RequestId id = 0;
	uint64_t owner = 0;
	RequestContext context;
	Response response;
	ResponseCallback callback;
	core::AtomicBool cancelled{false};
	core::AtomicBool done{false};
	bool started = false;
	void *transport = nullptr;
};

using AsyncRequestPtr = core::SharedPtr<AsyncRequest>;

bool initAsync();
void startAsync(const AsyncRequestPtr &request);
void cancelAsync(const AsyncRequestPtr &request);
void shutdownAsync();
void executeRequest(AsyncRequest &request);

} // namespace detail
} // namespace http
