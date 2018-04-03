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

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include "core/Concurrency.h"
#include "core/String.h"

namespace ai {

class ThreadPool final {
public:
	explicit ThreadPool(size_t);

	/**
	 * Enqueue functors or lambdas into the thread pool
	 */
	template<class F, class ... Args>
	auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

	~ThreadPool();
private:
	// need to keep track of threads so we can join them
	std::vector<std::thread> _workers;
	// the task queue
	std::queue<std::function<void()> > _tasks;

	// synchronization
	std::mutex _queueMutex;
	std::condition_variable _condition;
	std::atomic_bool _stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads) :
		_stop(false) {
	_workers.reserve(threads);
	for (size_t i = 0; i < threads; ++i) {
		_workers.emplace_back([this, i] {
			const std::string n = core::string::format("SimpleAI-%i", (int)i);
			core::setThreadName(n.c_str());
			for (;;) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->_queueMutex);
					this->_condition.wait(lock, [this] {
						return this->_stop || !this->_tasks.empty();
					});
					if (this->_stop && this->_tasks.empty()) {
						return;
					}
					task = std::move(this->_tasks.front());
					this->_tasks.pop();
				}

				task();
			}
		});
	}
}

// add new work item to the pool
template<class F, class ... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type> {
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared<std::packaged_task<return_type()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(_queueMutex);
		_tasks.emplace([task]() {(*task)();});
		_condition.notify_one();
	}
	return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
	_stop = true;
	_condition.notify_all();
	for (std::thread &worker : _workers)
		worker.join();
}

}
