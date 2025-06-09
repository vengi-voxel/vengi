/**
 * @file
 */

#pragma once

#include <future>
#include <chrono>

namespace core {

template<typename T>
class Future {
private:
	std::future<T> _future;

public:
	constexpr Future() {
	}

	Future(std::future<T> future) : _future(std::move(future)) {
	}

	bool valid() const {
		return _future.valid();
	}

	T get() {
		return _future.get();
	}

	bool ready() const {
		if (!valid()) {
			return false;
		}
		return _future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}

	void wait() {
		if (!valid()) {
			return;
		}
		_future.wait();
	}
};

} // namespace core
