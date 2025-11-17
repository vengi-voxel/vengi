/**
 * @file
 */

#include "Async.h"
#include "app/App.h"
#include "core/collection/DynamicArray.h"

namespace app {

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

void for_not_parallel(int start, int end, const std::function<void(int, int)> &f) {
	core_trace_scoped(for_not_parallel);
	if (start >= end)
		return;
	f(start, end);
}

void for_parallel(int start, int end, const std::function<void(int, int)> &taskLambda, bool wait) {
	core_trace_scoped(for_parallel);
	if (start >= end)
		return;

	const int threadPoolSize = app::App::getInstance()->threads();
	if (end - start == 1 || threadPoolSize <= 1) {
		// if we are not on the main thread, we can just call the taskLambda directly
		taskLambda(start, end);
		return;
	}

	const int threadCnt = core_max(2, threadPoolSize);
	const int chunkSize = core_max((end - start + threadCnt - 1) / threadCnt, 1);
	const int taskCnt = (end - start) / chunkSize + 1;
	core::DynamicArray<core::Future<void>> futures;
	futures.reserve(taskCnt);

	for (int i = start; i < end; i += chunkSize) {
		uint32_t chunk_end = core_min(i + chunkSize, end);
		futures.emplace_back(app::App::getInstance()->enqueue([i, chunk_end, &taskLambda]() { taskLambda(i, chunk_end); }));
	}

	if (!wait) {
		return;
	}
	for (auto &fut : futures) {
		fut.wait();
	}
}

void schedule(std::function<void()> &&f) {
	app::App::getInstance()->schedule(core::forward<std::function<void()>>(f));
}

} // namespace app
