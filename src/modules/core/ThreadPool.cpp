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
	_force = false;
	_stop = false;
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
					if (!this->_stop) {
						this->_queueCondition.wait(lock, [this] {
							// predicate must return false if the waiting should continue
							if (this->_stop) {
								return true;
							}
							if (!this->_tasks.empty()) {
								return true;
							}
							return false;
						});
					}
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
	for (std::thread &worker : _workers) {
		worker.join();
	}
	_workers.clear();
}

}
