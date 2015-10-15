#pragma once

#include "Common.h"
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace core {

class ThreadPool {
private:
	typedef std::function<void()> FunctionType;
	bool _join;
	std::vector<std::thread> _workers;
	std::deque<FunctionType> _tasks;
	std::condition_variable _cond;
	std::mutex _mutex;

	void tick();
public:
	ThreadPool(unsigned int numThreads = 2) :
			_join(false) {
		for (unsigned int i = 0; i < numThreads; ++i) {
			_workers.push_back(std::move(std::thread(std::bind(&ThreadPool::tick, this))));
		}
	}

	template<class Function, class ... Args>
	auto push(Function&& func, Args&&... args) -> std::future<typename std::result_of<Function(Args...)>::type> {
		core_assert(!_join);
		using retType = typename std::result_of<Function(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<retType()> >(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
		std::future<retType> res = task->get_future();

		auto funcWrapper = [task]() {
			(*task)();
		};
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_tasks.push_back(funcWrapper);
		}
		_cond.notify_one();
		return res;
	}

	~ThreadPool() {
		_join = true;
		_cond.notify_all();
		for (std::thread &worker : _workers) {
			worker.join();
		}
	}
};

}
