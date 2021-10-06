#pragma once

#include "core/NonCopyable.h"

namespace core {

template<typename T>
class ScopedPtr : public core::NonCopyable {
private:
	T *_ptr;

public:
	explicit ScopedPtr(T *const ptr = nullptr) : _ptr(ptr) {
	}

	~ScopedPtr() {
		if (_ptr) {
			delete _ptr;
		}
	}

	void operator=(T *const p) {
		if (_ptr) {
			delete _ptr;
		}
		_ptr = p;
	}

	T *operator->() const {
		return _ptr;
	}

	T &operator*() const {
		return *_ptr;
	}

	operator bool() const {
		return _ptr;
	}

	operator T *() const {
		return _ptr;
	}

	T *release() {
		T *const p = _ptr;
		_ptr = nullptr;
		return p;
	}

	void deallocate() {
		*this = nullptr;
	}
};

}
