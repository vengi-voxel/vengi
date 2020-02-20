/**
 * @file
 */

#pragma once

#include "core/Atomic.h"
#include "core/Assert.h"
#include "core/Common.h"
#include <SDL_stdinc.h>
#include <new>

namespace core {

template<class T>
class SharedPtr {
private:
	T *_ptr;
	core::AtomicInt *_refCnt;

	int count() const {
		if (_refCnt == nullptr) {
			return 0;
		}
		return *_refCnt;
	}

	void increase() {
		if (_refCnt == nullptr) {
			return;
		}
		++(*_refCnt);
	}

	void decrease() {
		if (_refCnt == nullptr) {
			return;
		}
		--(*_refCnt);
	}
public:
	constexpr SharedPtr() : _ptr(nullptr), _refCnt(nullptr) {
	}

	SharedPtr(const SharedPtr &obj) : _ptr(obj._ptr), _refCnt(obj._refCnt) {
		increase();
	}

	SharedPtr(SharedPtr &&obj) : _ptr(obj._ptr), _refCnt(obj._refCnt) {
		obj._ptr = nullptr;
		obj._refCnt = nullptr;
	}

	template<typename ... Args>
	static SharedPtr<T> create(Args&&... args) {
		const size_t size = sizeof(T) + sizeof(AtomicInt);
		void *ptr = core_malloc(size);
		SharedPtr<T> d;
		d._ptr = new (ptr) T(core::forward<Args>(args)...);
		d._refCnt = new ((void*)((uint8_t*)ptr + sizeof(*_ptr))) AtomicInt(1);
		return d;
	}

	SharedPtr &operator=(const SharedPtr &obj) {
		_ptr = obj._ptr;
		_refCnt = obj._refCnt;
		increase();
		return *this;
	}

	SharedPtr &operator=(SharedPtr &&obj) {
		_ptr = obj._ptr;
		_refCnt = obj._refCnt;
		obj._ptr = nullptr;
		obj._refCnt = nullptr;
		return *this;
	}

	~SharedPtr() {
		release();
	}

	void release() {
		decrease();
		if (count() == 0) {
			if (_ptr != nullptr) {
				_ptr->~T();
			}
			if (_refCnt != nullptr) {
				_refCnt->~AtomicInt();
			}
			core_free((void*)_ptr);
		}
	}

	T *get() const {
		return _ptr;
	}

	T *operator->() const {
		return _ptr;
	}

	void operator=(std::nullptr_t) {
		release();
	}

	bool operator==(const SharedPtr &rhs) const {
		return _ptr == rhs._ptr;
	}

	bool operator!=(const SharedPtr &rhs) const {
		return _ptr != rhs._ptr;
	}

	bool operator<(const SharedPtr &rhs) const {
		return _ptr < rhs._ptr;
	}

	bool operator>(const SharedPtr &rhs) const {
		return _ptr > rhs._ptr;
	}

	bool operator<=(const SharedPtr &rhs) const {
		return _ptr <= rhs._ptr;
	}

	bool operator>=(const SharedPtr &rhs) const {
		return _ptr >= rhs._ptr;
	}

	bool operator==(std::nullptr_t) const {
		return nullptr == _ptr;
	}

	bool operator!=(std::nullptr_t) const {
		return nullptr != _ptr;
	}
};

}
