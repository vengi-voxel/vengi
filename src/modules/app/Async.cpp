/**
 * @file
 */

#include "Async.h"
#include "core/collection/DynamicArray.h"

namespace app {

void for_parallel(int start, int end, const std::function<void(int, int)> &taskLambda) {
	if (start >= end)
		return;

	core::ThreadPool &threadPool = app::App::getInstance()->threadPool();
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
