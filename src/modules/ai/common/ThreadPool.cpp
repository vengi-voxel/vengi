/**
 * @file
 */

#include "ThreadPool.h"

namespace ai {

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads) {
	_workers.reserve(threads);
	for (size_t i = 0; i < threads; ++i) {
		_workers.emplace_back([this, i] {
			const std::string n = core::string::format("SimpleAI-%i", (int)i);
			core::setThreadName(n.c_str());
			for (;;) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->_queueMutex);
					this->_queueCondition.wait(lock, [this] {
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

ThreadPool::~ThreadPool() {
	_stop = true;
	_queueCondition.notify_all();
	for (std::thread &worker : _workers) {
		worker.join();
	}
	_workers.clear();
}

}
