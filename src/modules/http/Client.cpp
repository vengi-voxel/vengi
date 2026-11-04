/**
 * @file
 */

#include "Client.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "system/Async.h"

namespace http {
namespace {

struct State {
	core_trace_mutex(core::Lock, mutex, "HttpClient");
	uint64_t nextOwner = 0;
	RequestId nextRequest = 0;
	bool initialized = false;
	bool updating = false;
	core::DynamicArray<detail::AsyncRequestPtr> requests;
};

State &state() {
	static State s;
	return s;
}

core::DynamicArray<detail::AsyncRequestPtr> pendingRequests() {
	State &s = state();
	core::ScopedLock lock(s.mutex);
	return s.requests;
}

void cancelRequest(const detail::AsyncRequestPtr &request) {
	request->cancelled = true;
	request->callback = nullptr;
	detail::cancelAsync(request);
}

} // namespace

Client::Client() {
	State &s = state();
	core::ScopedLock lock(s.mutex);
	_id = ++s.nextOwner;
}

Client::~Client() {
	cancelAll();
}

RequestId Client::execute(const Request &request, ResponseCallback callback) {
	State &s = state();
	core::ScopedLock lock(s.mutex);
	if (!s.initialized || !Request::supported() || s.requests.size() >= 64) {
		return 0;
	}
	auto pending = core::make_shared<detail::AsyncRequest>();
	pending->id = ++s.nextRequest;
	pending->owner = _id;
	pending->context = request._ctx;
	pending->context._cancelled = &pending->cancelled;
	pending->response.url = request.url();
	pending->callback = core::move(callback);
	s.requests.push_back(pending);
	return pending->id;
}

bool Client::cancel(RequestId id) {
	for (const auto &request : pendingRequests()) {
		if (request->id == id && request->owner == _id && !request->cancelled) {
			cancelRequest(request);
			return true;
		}
	}
	return false;
}

void Client::cancelAll() {
	for (const auto &request : pendingRequests()) {
		if (request->owner == _id && !request->cancelled) {
			cancelRequest(request);
		}
	}
}

void init() {
	State &s = state();
	core::ScopedLock lock(s.mutex);
	if (s.initialized) {
		return;
	}
	s.initialized = Request::supported() && detail::initAsync();
}

void update() {
	State &s = state();
	if (s.updating) {
		return;
	}
	s.updating = true;
	// Snapshot the list so callbacks may submit requests. Keep pending entries in
	// the registry until dispatch so one callback can cancel another completed one.
	const auto requests = pendingRequests();
	for (const auto &request : requests) {
		if (request->cancelled) {
			continue;
		}
		if (!request->started) {
			request->started = true;
			detail::startAsync(request);
		}
		if (!request->done) {
			continue;
		}
		ResponseCallback callback = core::move(request->callback);
		request->cancelled = true;
		detail::cancelAsync(request);
		if (callback) {
			callback(request->response);
		}
	}
	{
		core::ScopedLock lock(s.mutex);
		s.requests.erase_if([](const detail::AsyncRequestPtr &request) { return (bool)request->cancelled; });
	}
	s.updating = false;
}

void shutdown() {
	State &s = state();
	core::DynamicArray<detail::AsyncRequestPtr> requests;
	{
		core::ScopedLock lock(s.mutex);
		if (!s.initialized) {
			return;
		}
		s.initialized = false;
		requests = core::move(s.requests);
	}
	for (const auto &request : requests) {
		cancelRequest(request);
	}
	detail::shutdownAsync();
}

} // namespace http
