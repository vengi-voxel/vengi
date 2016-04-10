// Based on http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
//
// Note from the comments on licensing: "Yes, you can just copy the code presented here and use
// it for whatever you like. There won't be any licensing issues. I'm glad you find it helpful."

#pragma once

#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Cubiquity {

template<class Data, class Compare>
class concurrent_queue {
private:
	std::priority_queue<Data, std::vector<Data>, Compare> the_queue;
	mutable std::mutex the_mutex;
	std::condition_variable the_condition_variable;
public:
	void push(Data const& data) {
		the_mutex.lock();
		the_queue.push(data);
		the_mutex.unlock();
		the_condition_variable.notify_one();
	}

	void push(Data&& data) {
		the_mutex.lock();
		the_queue.emplace(std::forward<Data>(data));
		the_mutex.unlock();
		the_condition_variable.notify_one();
	}

	bool empty() const {
		std::unique_lock<std::mutex> lock(the_mutex);
		return the_queue.empty();
	}

	uint32_t size() const {
		std::unique_lock<std::mutex> lock(the_mutex);
		return the_queue.size();
	}

	bool try_pop(Data& popped_value) {
		std::unique_lock<std::mutex> lock(the_mutex);
		if (the_queue.empty()) {
			return false;
		}

		popped_value = the_queue.top();
		the_queue.pop();
		return true;
	}

	void wait_and_pop(Data& popped_value) {
		std::unique_lock<std::mutex> lock(the_mutex);
		while (the_queue.empty()) {
			the_condition_variable.wait(lock);
		}

		popped_value = the_queue.top();
		the_queue.pop();
	}
};

}
