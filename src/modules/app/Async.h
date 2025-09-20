/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/concurrent/Future.h"

namespace app {

// add new work item to the pool
// TODO: get rid of the futures here - use schedule or for_parallel - this would allow us to hide the thread pool
// implementation and make it easier to switch to a different implementation that might also not rely on the STL
template<class F>
auto async(F &&f) -> core::Future<typename std::invoke_result<F>::type> {
	return app::App::getInstance()->enqueue(core::forward<F>(f));
}

// add new work item to the pool
template<class F>
void schedule(F &&f) {
	app::App::getInstance()->enqueue(core::forward<F>(f));
}

void for_not_parallel(int start, int end, const std::function<void(int, int)> &f);
void for_parallel(int start, int end, const std::function<void(int, int)> &f, bool wait = true);
int for_parallel_size(int start, int end);

} // namespace app
