/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/concurrent/ThreadPool.h"
#include <future>

namespace app {

// add new work item to the pool
template<class F, class... Args>
auto async(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type> {
	return app::App::getInstance()->threadPool().enqueue(core::forward<F>(f), core::forward<Args>(args)...);
}

} // namespace app
