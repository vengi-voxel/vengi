/**
 * @file
 */

#pragma once

#include "core/Common.h"

namespace core {

template<typename T>
class Optional {
private:
	T *_value = nullptr;
	bool _owns = false;

	void release() {
		if (_owns) {
			delete _value;
		}
	}

public:
	Optional() {
	}

	~Optional() {
		release();
	}

	Optional &operator=(Optional &&move) noexcept {
		if (&move == this) {
			return *this;
		}
		release();
		_owns = move._owns;
		_value = move._value;
		move._owns = false;
		move._value = nullptr;
		return *this;
	}

	Optional(Optional&& move) noexcept {
		_owns = move._owns;
		_value = move._value;
		move._owns = false;
		move._value = nullptr;
	}

	inline const T *value() const {
		return _value;
	}

	inline bool hasValue() const {
		return _value != nullptr;
	}

	void setValue(T &&value) {
		release();
		_value = new T(core::forward<T>(value));
		_owns = true;
	}

	void setValue(const T &value) {
		release();
		_value = new T(value);
		_owns = true;
	}

	void setValue(T *value) {
		release();
		_value = value;
		_owns = false;
	}
};

} // namespace core
