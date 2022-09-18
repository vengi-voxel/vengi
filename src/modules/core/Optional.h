/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/StandardLib.h"

namespace core {

template<typename T>
class Optional {
private:
	T *_value = nullptr;
	bool _owns = false;
	bool _hasValue = false;

	void prepare() {
		if (_owns && _value != nullptr) {
			_value->~T();
		} else {
			_value = (T *)core_malloc(sizeof(T));
			_owns = true;
		}
	}

	void release() {
		if (_owns) {
			if (_value != nullptr) {
				_value->~T();
				core_free(_value);
				_owns = false;
				_value = nullptr;
			}
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
		_hasValue = move._hasValue;
		move._owns = false;
		move._value = nullptr;
		move._hasValue = false;
		return *this;
	}

	Optional(Optional &&move) noexcept {
		_owns = move._owns;
		_value = move._value;
		_hasValue = move._hasValue;
		move._owns = false;
		move._value = nullptr;
		move._hasValue = false;
	}

	Optional &operator=(const Optional &other) noexcept {
		if (&other == this) {
			return *this;
		}
		if (other.hasValue()) {
			_hasValue = false;
			return *this;
		}
		prepare();
		_value = new (_value) T(*other._value);
		_hasValue = _value != nullptr;
		return *this;
	}

	Optional(const Optional& other) noexcept {
		if (!other.hasValue()) {
			_hasValue = false;
			return;
		}
		prepare();
		_value = new (_value) T(*other._value);
		_hasValue = _value != nullptr;
	}

	inline T *value() {
		if (!_hasValue) {
			return nullptr;
		}
		return _value;
	}

	inline const T *value() const {
		if (!_hasValue) {
			return nullptr;
		}
		return _value;
	}

	inline bool hasValue() const {
		return _hasValue;
	}

	void setValue(T &&value) {
		prepare();
		_value = new (_value) T(core::forward<T>(value));
		_owns = true;
		_hasValue = true;
	}

	void setValue(const T &value) {
		prepare();
		_value = new (_value) T(value);
		_owns = true;
		_hasValue = true;
	}

	void setValue(T *value) {
		if (_value == value) {
			return;
		}
		release();
		_value = value;
		_owns = false;
		_hasValue = _value != nullptr;
	}
};

} // namespace core
