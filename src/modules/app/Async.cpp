/**
 * @file
 */

#include "Async.h"
#include "app/App.h"
#include "core/collection/DynamicArray.h"

namespace app {

void for_parallel(int start, int end, const std::function<void(int, int)> &taskLambda) {
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
	core::DynamicArray<core::Future<void>> futures;
	futures.reserve((end - start) / chunkSize + 1);

	for (int i = start; i < end; i += chunkSize) {
		uint32_t chunk_end = core_min(i + chunkSize, end);
		futures.emplace_back(app::App::getInstance()->enqueue([i, chunk_end, &taskLambda]() { taskLambda(i, chunk_end); }));
	}

	for (auto &fut : futures) {
		fut.wait();
	}
}

} // namespace app
