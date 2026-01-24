/**
 * @file
 */

/*
 Copyright (c) 2012 Jakob Progsch, Václav Zeman

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.

 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.

 3. This notice may not be removed or altered from any source
 distribution.
 */

#pragma once

#include <thread>
#include <functional>
#include "core/ResultType.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Queue.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Future.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ConditionVariable.h"
#include "core/Trace.h"
#include "core/SharedPtr.h"

namespace core {

class ThreadPool final {
public:
	explicit ThreadPool(size_t, const char *name = nullptr);
	~ThreadPool();

	/**
	 * Enqueue functors or lambdas into the thread pool
	 */
	template<class F>
	auto enqueue(F&& f) -> core::Future<core::invoke_result_t<F>>;
	void schedule(std::function<void()> &&f);

	void dump() const;
	size_t size() const;
	void init();
	/**
	 * @brief Remove queued and not yet executed tasks
	 * @note This does not abort the current running task
	 */
	void abort();
	void shutdown(bool wait = false);

	void reserve(size_t n);
private:
	static thread_local bool _inThreadPool;
	const size_t _threads;
	const char *_name;
	// need to keep track of threads so we can join them
	core::DynamicArray<std::thread> _workers;
	// the task queue
	core::Queue<std::function<void()> > _tasks core_thread_guarded_by(_queueMutex);

	// synchronization
	core_trace_mutex(core::Lock, _queueMutex, "ThreadPoolQueue");
	core::ConditionVariable _queueCondition;
	core::AtomicBool _stop { false };
	core::AtomicBool _force { false };
	core::AtomicInt _activeWorkers { 0 };
};

inline void ThreadPool::reserve(size_t n) {
	core::ScopedLock lock(_queueMutex);
	_tasks.reserve(n);
}

// add new work item to the pool
template<class F>
auto ThreadPool::enqueue(F&& f)
-> core::Future<core::invoke_result_t<F>> {
	using return_type = core::invoke_result_t<F>;
	if (_stop) {
		return core::Future<return_type>();
	}

	{
		core::ScopedLock lock(_queueMutex);

		if (_stop) {
			return core::Future<return_type>();
		}

		if (!_inThreadPool || (int)_activeWorkers < (int)_threads) {
			core::SharedPtr<std::packaged_task<return_type()>> task =
				core::make_shared<std::packaged_task<return_type()>>(
					std::bind(core::forward<F>(f)));

			core::Future<return_type> res = task->get_future();
			_tasks.emplace([task]() {(*task.get())();});
			_queueCondition.notify_one();
			return res;
		}
	}

	// If we are in a thread pool thread, execute the task inline if no free worker is available
	// this prevents the nested parallelism deadlock problem
	auto task = std::packaged_task<return_type()>(std::bind(core::forward<F>(f)));
	core::Future<return_type> res = task.get_future();
	task();
	return res;
}

inline size_t ThreadPool::size() const {
	return _threads;
}

}
