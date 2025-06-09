/**
 * @file
 */

#pragma once

#include "app/App.h"

namespace app {

// add new work item to the pool
// TODO: get rid of the futures here - use schedule or for_parallel - this would allow us to hide the thread pool
// implementation and make it easier to switch to a different implementation that might also not rely on the STL
template<class F>
auto async(F &&f) -> std::future<typename std::invoke_result<F>::type> {
	return app::App::getInstance()->enqueue(core::forward<F>(f));
}

// add new work item to the pool
template<class F>
void schedule(F &&f) {
	app::App::getInstance()->enqueue(core::forward<F>(f));
}

void for_parallel(int start, int end, const std::function<void(int, int)> &taskLambda);

} // namespace app
