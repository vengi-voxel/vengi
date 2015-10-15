#include "ThreadPool.h"

namespace core {

void ThreadPool::tick() {
	FunctionType task;
	for (;;) {
		{
			std::unique_lock<std::mutex> lock(_mutex);
			while (!_join && _tasks.empty()) {
				_cond.wait(lock);
			}
			if (_join) {
				core_assert(_tasks.empty());
				return;
			}

			task = _tasks.front();
			_tasks.pop_front();
		}

		task();
	}
}

}
