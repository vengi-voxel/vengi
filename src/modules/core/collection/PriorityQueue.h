/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include <algorithm>
#include <stdint.h>
#include <vector>

namespace core {

template<class Data, class Comparator = Less<Data>>
class PriorityQueue {
private:
	using Collection = std::vector<Data>;
	Collection _data;
	Comparator _comparator;

public:
	using value_type = Data;
	using Key = Data;

	PriorityQueue(size_t reserve = 0u) : _comparator(Comparator()) {
		if (reserve) {
			_data.reserve(reserve);
		}
	}
	PriorityQueue(Comparator comparator, size_t reserve = 0u) : _comparator(comparator) {
		if (reserve) {
			_data.reserve(reserve);
		}
	}

	void setComparator(Comparator comparator) {
		_comparator = comparator;
		if (_data.empty()) {
			return;
		}
		std::make_heap(const_cast<Data *>(&_data.front()), const_cast<Data *>(&_data.front()) + _data.size(),
					   _comparator);
	}

	void clear() {
		_data.clear();
	}

	void release() {
		_data.release();
	}

	void sort() {
		if (_data.empty()) {
			return;
		}
		std::make_heap(const_cast<Data *>(&_data.front()), const_cast<Data *>(&_data.front()) + _data.size(),
					   _comparator);
	}

	CORE_FORCE_INLINE const Data &operator[](size_t idx) const {
		core_assert_msg(idx < _data.size(), "idx is out of bounds: %i vs %i", (int)idx, (int)_data.size());
		return _data[idx];
	}

	Data &operator[](size_t idx) {
		core_assert_msg(idx < _data.size(), "idx is out of bounds: %i vs %i", (int)idx, (int)_data.size());
		return _data[idx];
	}

	void push(Data const &data) {
		_data.push_back(data);
		std::push_heap(_data.begin(), _data.end(), _comparator);
	}

	void push(Data &&data) {
		_data.push_back(core::move(data));
		std::push_heap(_data.begin(), _data.end(), _comparator);
	}

	template<typename... _Args>
	void emplace(_Args &&...__args) {
		_data.emplace_back(core::forward<_Args>(__args)...);
		std::push_heap(_data.begin(), _data.end(), _comparator);
	}

	CORE_FORCE_INLINE bool empty() const {
		return _data.empty();
	}

	CORE_FORCE_INLINE uint32_t size() const {
		return (uint32_t)_data.size();
	}

	bool pop(Data &poppedValue) {
		if (_data.empty()) {
			return false;
		}

		poppedValue = core::move(_data.front());
		std::pop_heap(_data.begin(), _data.end(), _comparator);
		_data.pop_back();
		return true;
	}
};

} // namespace core
