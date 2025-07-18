/**
 * @file
 */

#include "ThreadPool.h"
#include "core/String.h"
#include "core/Trace.h"
#include "core/Log.h"
#include "core/concurrent/Concurrency.h"

namespace core {

thread_local bool ThreadPool::_inThreadPool = false;

ThreadPool::ThreadPool(size_t threads, const char *name) :
		_threads(threads), _name(name) {
	if (_name == nullptr) {
		_name = "ThreadPool";
	}
}

void ThreadPool::abort() {
	core::ScopedLock lock(_queueMutex);
	_tasks.clear();
}

void ThreadPool::init() {
#ifdef __EMSCRIPTEN__
#ifndef __EMSCRIPTEN_PTHREADS__
#error "Compile with -pthread"
#endif
#endif

	_force = false;
	_stop = false;
	_workers.reserve(_threads);
	for (size_t i = 0; i < _threads; ++i) {
		_workers.emplace_back([this, i] {
			_inThreadPool = true;
			const core::String n = core::String::format("%s-%i", this->_name, (int)i);
			if (!setThreadName(n.c_str())) {
				Log::debug("Failed to set thread name for pool thread %i", (int)i);
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
							core::ScopedLock queueLock(this->_queueMutex);
							if (!this->_tasks.empty()) {
								return true;
							}
							return false;
						});
					}
					if (this->_stop && (this->_force || this->_tasks.empty())) {
						Log::debug("Shutdown worker thread for %i", (int)i);
						break;
					}
					task = core::move(this->_tasks.front());
					this->_tasks.pop();
					_activeWorkers.increment();
				}

				core_trace_begin_frame(n.c_str());
				core_trace_scoped(ThreadPoolWorker);
				Log::trace("Execute task in %i", (int)i);
				task();
				Log::trace("End of task in %i", (int)i);
				core_trace_end_frame(n.c_str());
				_activeWorkers.decrement();
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
