/**
 * @file
 */

#include "ThreadPool.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/concurrent/Concurrency.h"

namespace core {

ThreadPool::ThreadPool(size_t threads, const char *name) :
		_threads(threads), _name(name) {
	if (_name == nullptr) {
		_name = "ThreadPool";
	}
}

void ThreadPool::abort() {
	std::unique_lock lock(_queueMutex);
	while (!_tasks.empty()) {
		_tasks.pop();
	}
}

void ThreadPool::init() {
	_force = false;
	_stop = false;
	_workers.reserve(_threads);
	for (size_t i = 0; i < _threads; ++i) {
		_workers.emplace_back([this, i] {
			const core::String n = core::string::format("%s-%i", this->_name, (int)i);
			if (!setThreadName(n.c_str())) {
				Log::error("Failed to set thread name for pool thread %i", (int)i);
			}
			core_trace_thread(n.c_str());
			for (;;) {
				std::function<void()> task;
				{
					core::ScopedLock lock(this->_queueMutex);
					if (!this->_stop) {
						this->_queueCondition.wait(this->_queueMutex, [this] {
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
						Log::debug(logid, "Shutdown worker thread for %i", (int)getThreadId());
						break;
					}
					task = core::move(this->_tasks.front());
					this->_tasks.pop();
				}

				core_trace_begin_frame(n.c_str());
				core_trace_scoped(ThreadPoolWorker);
				Log::debug(logid, "Execute task in %i", (int)getThreadId());
				task();
				Log::debug(logid, "End of task in %i", (int)getThreadId());
				core_trace_end_frame(n.c_str());
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
