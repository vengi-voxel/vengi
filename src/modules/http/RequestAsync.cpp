/**
 * @file
 */

#include "RequestAsync.h"
#include "app/Async.h"
#include "core/Log.h"
#include "core/Pair.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/DynamicMap.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "io/BufferedReadWriteStream.h"

namespace http {

namespace {

struct CompletedRequest {
	uint32_t id = 0;
	Response response;
};

struct AsyncState {
	core::AtomicInt nextId{1};
	core_trace_mutex(core::Lock, mutex, "HttpRequestAsync");
	core::DynamicMap<uint32_t, ResponseCallback> callbacks core_thread_guarded_by(mutex);
	core::ConcurrentQueue<CompletedRequest> completed;
};

AsyncState &asyncState() {
	static AsyncState state;
	return state;
}

Response executeRequest(Request &request) {
	Response response;
	response.url = request.url();
	io::BufferedReadWriteStream stream(64 * 1024);
	response.success = request.execute(stream, &response.statusCode, &response.headers);
	if (stream.size() > 0) {
		// Keep the raw bytes (may contain embedded nulls; do not use readString).
		response.body.append((const char *)stream.getBuffer(), (size_t)stream.size());
	}
	return response;
}

} // namespace

uint32_t requestAsync(Request &&request, ResponseCallback &&callback) {
	if (!callback) {
		Log::error("http::requestAsync requires a callback");
		return 0u;
	}
	if (!Request::supported()) {
		Log::error("http::requestAsync: HTTP is not supported on this platform");
		return 0u;
	}

	AsyncState &state = asyncState();
	const uint32_t id = (uint32_t)state.nextId.increment(1);
	{
		core::ScopedLock lock(state.mutex);
		state.callbacks.emplace(id, core::move(callback));
	}

	app::schedule([id, request = core::move(request)]() mutable {
		CompletedRequest completed;
		completed.id = id;
		completed.response = executeRequest(request);
		asyncState().completed.push(core::move(completed));
	});
	return id;
}

uint32_t requestAsync(const core::String &url, RequestType type, ResponseCallback &&callback) {
	return requestAsync(Request(url, type), core::move(callback));
}

void update() {
	AsyncState &state = asyncState();
	core::DynamicArray<CompletedRequest> done;
	if (!state.completed.popAll(done)) {
		return;
	}

	for (CompletedRequest &entry : done) {
		ResponseCallback callback;
		{
			core::ScopedLock lock(state.mutex);
			if (!state.callbacks.get(entry.id, callback)) {
				Log::debug("http::update: no callback for request %u (cleared?)", entry.id);
				continue;
			}
			state.callbacks.remove(entry.id);
		}
		callback(entry.response);
	}
}

void clearPending() {
	AsyncState &state = asyncState();
	{
		core::ScopedLock lock(state.mutex);
		state.callbacks.clear();
	}
	state.completed.clear();
}

} // namespace http
