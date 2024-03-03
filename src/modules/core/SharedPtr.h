/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <new>
#include <stddef.h>

namespace core {

template<class T>
class SharedPtr {
private:
	template <typename U> friend class SharedPtr;

	struct __enableIfHelper {
		int b;
	};

	struct Data {
		T *_ptr;
		core::AtomicInt *_refCnt;
	};
	Data *_data;

	int count() const {
		if (_data == nullptr) {
			return 0;
		}
		return *_data->_refCnt;
	}

	void increase() {
		if (_data == nullptr) {
			return;
		}
		_data->_refCnt->increment(1);
	}

	int decrease() {
		if (_data == nullptr) {
			return -1;
		}
		return _data->_refCnt->decrement(1) - 1;
	}
public:
	constexpr SharedPtr() : _data(nullptr) {
	}

	constexpr SharedPtr(decltype(nullptr)) : _data(nullptr) {
	}

	SharedPtr(const SharedPtr &obj) : _data(obj._data) {
		increase();
	}

	SharedPtr(SharedPtr &&obj) noexcept : _data(obj._data) {
		obj._data = nullptr;
	}

	template <class U>
	SharedPtr(const SharedPtr<U> &obj, typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type = __enableIfHelper()) {
		_data = (Data*)(void*)obj._data;
		increase();
	}

	template <class U>
	SharedPtr(SharedPtr<U> &&obj, typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type = __enableIfHelper())
			 {
		_data = (Data*)(void*)obj._data;
		obj._data = nullptr;
	}

	template<typename ... Args>
	static SharedPtr<T> create(Args&&... args) {
		const size_t size = sizeof(Data) + sizeof(T) + sizeof(AtomicInt);
		void *ptr = core_malloc(size);
		SharedPtr<T> d;
		d._data = (Data*)ptr;
		d._data->_ptr = new ((uint8_t*)ptr + sizeof(Data)) T(core::forward<Args>(args)...);
		d._data->_refCnt = new ((uint8_t*)ptr + sizeof(Data) + sizeof(T)) AtomicInt(1);
		return d;
	}

	SharedPtr &operator=(const SharedPtr &obj) {
		if (&obj == this) {
			return *this;
		}
		release();
		_data = obj._data;
		increase();
		return *this;
	}

	SharedPtr &operator=(SharedPtr &&obj) noexcept {
		release();
		_data = obj._data;
		obj._data = nullptr;
		return *this;
	}

	~SharedPtr() {
		release();
	}

	core::AtomicInt* refCnt() const {
		if (_data == nullptr) {
			return nullptr;
		}
		return _data->_refCnt;
	}

	void release() {
		if (decrease() == 0) {
			if (_data != nullptr) {
				_data->_ptr->~T();
			}
			if (_data->_refCnt != nullptr) {
				_data->_refCnt->~AtomicInt();
			}
			core_free((void*)_data);
		}
		_data = nullptr;
	}

	T *get() const {
		if (_data == nullptr) {
			return nullptr;
		}
		return _data->_ptr;
	}

	T *operator->() const {
		return _data->_ptr;
	}

	void operator=(decltype(nullptr)) {
		release();
	}

	operator bool() const {
		return *this != nullptr;
	}

	bool operator==(const SharedPtr &rhs) const {
		return _data == rhs._data;
	}

	bool operator!=(const SharedPtr &rhs) const {
		return _data != rhs._data;
	}

	bool operator<(const SharedPtr &rhs) const {
		return _data < rhs._data;
	}

	bool operator>(const SharedPtr &rhs) const {
		return _data > rhs._data;
	}

	bool operator<=(const SharedPtr &rhs) const {
		return _data <= rhs._data;
	}

	bool operator>=(const SharedPtr &rhs) const {
		return _data >= rhs._data;
	}

	bool operator==(decltype(nullptr)) const {
		return nullptr == _data;
	}

	bool operator!=(decltype(nullptr)) const {
		return nullptr != _data;
	}
};

template<class T, typename ... Args>
static SharedPtr<T> make_shared(Args&&... args) {
	return SharedPtr<T>::create(core::forward<Args>(args)...);
}

}
