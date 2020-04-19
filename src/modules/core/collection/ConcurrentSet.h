/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <unordered_set>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <algorithm>
#include "core/Trace.h"

namespace collection {

template<class Data>
class ConcurrentSet {
public:
	using underlying_type = std::unordered_set<Data>;
private:
	underlying_type _data;
	mutable core_trace_mutex(std::mutex, _mutex);
	std::condition_variable_any _conditionVariable;
public:
	void swap(underlying_type& target) {
		std::unique_lock lock(_mutex);
		std::swap(_data, target);
	}

	void clear() {
		std::unique_lock lock(_mutex);
		_data.clear();
	}

	bool insert(Data const& data) {
		bool result;
		{
			std::unique_lock lock(_mutex);
			result = _data.insert(data).second;
		}
		_conditionVariable.notify_one();
		return result;
	}

	bool insert(Data&& data) {
		bool result;
		{
			std::unique_lock lock(_mutex);
			result = _data.insert(data).second;
		}
		_conditionVariable.notify_one();
		return result;
	}

	bool contains(Data const& data) const {
		std::unique_lock lock(_mutex);
		return _data.find(data) != _data.end();
	}

	inline bool empty() const {
		std::unique_lock lock(_mutex);
		return _data.empty();
	}

	inline uint32_t size() const {
		std::unique_lock lock(_mutex);
		return _data.size();
	}

	template<class VISITOR>
	void visit(VISITOR&& visitor) const {
		std::unique_lock lock(_mutex);
		for (const Data& d : _data) {
			visitor(d);
		}
	}
};

}
