/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/ResultType.h"
#include "core/concurrent/Future.h"

namespace app {

// add new work item to the pool
// TODO: get rid of the futures here - use schedule or for_parallel - this would allow us to hide the thread pool
// implementation and make it easier to switch to a different implementation that might also not rely on the STL
template<class F>
auto async(F &&f) -> core::Future<core::invoke_result_t<F>> {
	return app::App::getInstance()->enqueue(core::forward<F>(f));
}

// add new work item to the pool
void schedule(std::function<void()> &&f);

void for_not_parallel(int start, int end, const std::function<void(int, int)> &f);
void for_parallel(int start, int end, const std::function<void(int, int)> &f, bool wait = true);
int for_parallel_size(int start, int end);

/**
 * @note This sort is unstable
 */
template<typename Iter, class Comparator>
void sort_parallel(Iter first, Iter last, Comparator comp) {
	core_trace_scoped(sort_parallel);
	const int size = core::distance(first, last);
	if (size <= 128) {
		core::sort(first, last, comp);
		return;
	}

	const int chunkCount = app::for_parallel_size(0, size);
	if (chunkCount <= 1) {
		core::sort(first, last, comp);
		return;
	}

	const int chunkSize = (size + chunkCount - 1) / chunkCount;

	app::for_parallel(0, chunkCount, [first, &comp, chunkSize, size](int chunkIdx, int chunkIdxEnd) {
		for (int idx = chunkIdx; idx < chunkIdxEnd; ++idx) {
			const int start = idx * chunkSize;
			const int end = core_min(start + chunkSize, size);
			if (start < size) {
				const Iter chunkFirst = core::next(first, start);
				const Iter chunkLast = core::next(first, end);
				core::sort(chunkFirst, chunkLast, comp);
			}
		}
	});

	int currentChunkSize = chunkSize;
	while (currentChunkSize < size) {
		const int mergeSize = currentChunkSize * 2;

		// Merge pairs of chunks
		for (int i = 0; i + currentChunkSize < size; i += mergeSize) {
			const Iter leftFirst = core::next(first, i);
			const Iter middle = core::next(first, core_min(i + currentChunkSize, size));
			const Iter rightLast = core::next(first, core_min(i + mergeSize, size));
			core::inplace_merge(leftFirst, middle, rightLast, comp);
		}

		currentChunkSize = mergeSize;
	}
}

} // namespace app
