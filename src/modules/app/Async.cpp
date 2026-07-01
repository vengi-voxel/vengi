/**
 * @file
 */

#include "Async.h"
#include "app/App.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/ThreadPool.h"
#include "core/Log.h"
#include <SDL3/SDL_init.h>

namespace app {

namespace {

struct MainThreadCall {
	core::Function<void()> fn;
	bool destroyAfterCall = false;
};

static void SDLCALL runOnMainThreadCallback(void *userdata) {
	MainThreadCall *call = static_cast<MainThreadCall *>(userdata);
	call->fn();
	if (call->destroyAfterCall) {
		delete call;
	}
}

} // namespace

int for_parallel_size(int start, int end) {
	if (start >= end) {
		return 0;
	}
	const int threadPoolSize = app::App::getInstance()->threads();
	if (end - start == 1 || threadPoolSize <= 1) {
		return 1;
	}
	const int threadCnt = core_max(2, threadPoolSize);
	const int chunkSize = core_max((end - start + threadCnt - 1) / threadCnt, 1);
	return (end - start) / chunkSize + 1;
}

void for_not_parallel_impl(int start, int end, ForParallelFunc fn, void *ctx) {
	core_trace_scoped(for_not_parallel);
	if (start >= end)
		return;
	fn(ctx, start, end);
}

void for_parallel_impl(int start, int end, ForParallelFunc fn, void *ctx, bool wait) {
	core_trace_scoped(for_parallel);
	if (start >= end)
		return;

	const int threadPoolSize = app::App::getInstance()->threads();
	if (end - start == 1 || threadPoolSize <= 1) {
		// if we are not on the main thread, we can just call the taskLambda directly
		fn(ctx, start, end);
		return;
	}

	const int threadCnt = core_max(2, threadPoolSize);
	const int chunkSize = core_max((end - start + threadCnt - 1) / threadCnt, 1);
	const int taskCnt = (end - start) / chunkSize + 1;
	core::DynamicArray<core::Future<void>> futures;
	futures.reserve(taskCnt);

	for (int i = start; i < end; i += chunkSize) {
		uint32_t chunk_end = core_min(i + chunkSize, end);
		futures.emplace_back(app::App::getInstance()->threadPool()->enqueue([i, chunk_end, fn, ctx]() { fn(ctx, i, chunk_end); }));
	}

	if (!wait) {
		return;
	}
	for (auto &fut : futures) {
		fut.wait();
	}
}

void schedule(core::Function<void()> &&f) {
	app::App::getInstance()->schedule(core::forward<core::Function<void()>>(f));
}

bool runOnMainThread(core::Function<void()> &&f, bool waitComplete) {
	if (SDL_IsMainThread()) {
		f();
		return true;
	}

	if (waitComplete) {
		MainThreadCall call;
		call.fn = core::forward<core::Function<void()>>(f);
		if (!SDL_RunOnMainThread(runOnMainThreadCallback, &call, true)) {
			Log::error("Failed to schedule callback on main thread: %s", SDL_GetError());
			return false;
		}
		return true;
	}

	MainThreadCall *call = new MainThreadCall();
	call->fn = core::forward<core::Function<void()>>(f);
	call->destroyAfterCall = true;
	if (!SDL_RunOnMainThread(runOnMainThreadCallback, call, false)) {
		Log::error("Failed to schedule callback on main thread: %s", SDL_GetError());
		delete call;
		return false;
	}
	return true;
}

} // namespace app
