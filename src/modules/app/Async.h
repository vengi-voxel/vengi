/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/concurrent/ThreadPool.h"
#include <future>

namespace app {

// add new work item to the pool
// TODO: get rid of the futures here - use schedule or for_parallel - this would allow us to hide the thread pool
// implementation and make it easier to switch to a different implementation that might also not rely on the STL
template<class F, class... Args>
auto async(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type> {
	return app::App::getInstance()->threadPool().enqueue(core::forward<F>(f), core::forward<Args>(args)...);
}

// add new work item to the pool
template<class F>
void schedule(F &&f) {
	app::App::getInstance()->threadPool().enqueue(core::forward<F>(f));
}

void for_parallel(int start, int end, const std::function<void(int, int)> &taskLambda);

} // namespace app
