/**
 * @file
 */

#include "ThreadPool.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/Trace.h"
#include "core/concurrent/Concurrency.h"

namespace core {

thread_local bool ThreadPool::_inThreadPool = false;

ThreadPool::ThreadPool(size_t threads, const char *name) : _threads(threads), _name(name) {
	if (_name == nullptr) {
		_name = "ThreadPool";
	}
}

void ThreadPool::abort() {
	{
		core::ScopedLock lock(_queueMutex);
		_tasks.clear();
	}
	// wake workers so they can notice abort / stop condition
	_queueCondition.notify_all();
}

void ThreadPool::dump() const {
	size_t queued = 0;
	int threads = 0;
	int active = 0;
	{
		core::ScopedLock lock(_queueMutex);
		queued = _tasks.size();
		threads = (int)_threads;
		active = (int)_activeWorkers;
	}
	Log::info("ThreadPool '%s' dump: %d threads, %zu queued tasks, %d active workers", _name, threads, queued, active);
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
				bool shouldExit = false;
				bool haveActive = false;

				{
					core::ScopedLock lock(this->_queueMutex);
					// wait until stop or a task is available
					this->_queueCondition.wait(this->_queueMutex,
											   [this] { return this->_stop || !this->_tasks.empty(); });

					if (this->_stop && (this->_force || this->_tasks.empty())) {
						shouldExit = true;
					} else if (!this->_tasks.empty()) {
						task = core::move(this->_tasks.front());
						this->_tasks.pop();
						this->_activeWorkers.increment();
						haveActive = true;
					}
				}

				if (shouldExit) {
					Log::debug("Shutdown worker thread for %i", (int)i);
					break;
				}

				if (!task) {
					// spurious wake or no task to run
					continue;
				}

				core_trace_begin_frame(n.c_str());
				core_trace_scoped(ThreadPoolWorker);
				Log::trace("Execute task in %i", (int)i);
				task();
				Log::trace("End of task in %i", (int)i);
				core_trace_end_frame(n.c_str());

				if (haveActive) {
					this->_activeWorkers.decrement();
				}
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
	if (_force) {
		core::ScopedLock lock(_queueMutex);
		_tasks.clear();
	}
	_queueCondition.notify_all();
	for (std::thread &worker : _workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
	_workers.clear();
}

} // namespace core
