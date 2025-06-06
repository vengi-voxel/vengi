/**
 * @file
 */

#include "Async.h"
#include "app/App.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Thread.h"

namespace app {

void for_parallel(int start, int end, const std::function<void(int, int)> &taskLambda) {
	if (start >= end)
		return;

	core::ThreadPool &threadPool = app::App::getInstance()->threadPool();
	// if we are already running in a thread - we might be running in the thread pool - and as the threads in this pool are limited,
	// we might wait endlessly because these enqueue calls would not get executed.
	if (end - start == 1 || threadPool.size() == 1u || !app::App::getInstance()->isMainThread(core::getCurrentThreadId())) {
		// if we are not on the main thread, we can just call the taskLambda directly
		taskLambda(start, end);
		return;
	}

	const int threadCnt = core_max(2u, threadPool.size());
	const int chunkSize = core_max((end - start + threadCnt - 1) / threadCnt, 1);
	core::DynamicArray<std::future<void>> futures;
	futures.reserve((end - start) / chunkSize + 1);

	for (int i = start; i < end; i += chunkSize) {
		uint32_t chunk_end = core_min(i + chunkSize, end);
		futures.emplace_back(threadPool.enqueue([i, chunk_end, &taskLambda]() { taskLambda(i, chunk_end); }));
	}

	for (auto &fut : futures) {
		if (fut.valid()) {
			fut.wait();
		}
	}
}

} // namespace app
