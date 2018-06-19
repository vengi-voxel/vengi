/**
 * @file
 */

#include "ThreadPool.h"
#include "core/String.h"
#include "core/Trace.h"
#include "core/Concurrency.h"

namespace core {

ThreadPool::ThreadPool(size_t threads, const char *name) :
		_threads(threads), _name(name) {
	if (_name == nullptr) {
		_name = "ThreadPool";
	}
}

void ThreadPool::init() {
	_workers.reserve(_threads);
	for (size_t i = 0; i < _threads; ++i) {
		_workers.emplace_back([this, i] {
			const std::string n = core::string::format("%s-%i", this->_name, (int)i);
			setThreadName(n.c_str());
			core_trace_thread(n.c_str());
			for (;;) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->_queueMutex);
					this->_queueCondition.wait(lock, [this] {
						return this->_stop || !this->_tasks.empty();
					});
					if (this->_stop && (this->_force || this->_tasks.empty())) {
						break;
					}
					task = std::move(this->_tasks.front());
					this->_tasks.pop();
				}

				core_trace_begin_frame();
				core_trace_scoped(ThreadPoolWorker);
				task();
				core_trace_end_frame();
			}
			++_shutdownCount;
			_shutdownCondition.notify_all();
		});
	}
}

ThreadPool::~ThreadPool() {
	shutdown();
}

void ThreadPool::shutdown(bool wait) {
	if (_stop) {
		return;
	}
	_force = !wait;
	_stop = true;
	_queueCondition.notify_all();
	{
		std::unique_lock<std::mutex> lock(_shutdownMutex);
		_shutdownCondition.wait(lock, [&] { return _shutdownCount == (int)_workers.size(); });
	}
	for (std::thread &worker : _workers) {
		worker.join();
	}
	_workers.clear();
}

}
