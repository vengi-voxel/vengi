/**
 * @file
 */

#include "DynamicBitSet.h"
#include "core/Common.h"
#include "core/StandardLib.h"

namespace core {

DynamicBitSet::DynamicBitSet(size_t size) : _size(size) {
	if (_size > 0) {
		const size_t elements = requiredElements(_size);
		_buffer = (Type *)core_malloc(elements * sizeof(Type));
		clear();
	}
}

DynamicBitSet::~DynamicBitSet() {
	core_free(_buffer);
	_buffer = nullptr;
}

DynamicBitSet::DynamicBitSet(const DynamicBitSet &other) : _size(other._size) {
	if (_size > 0) {
		const size_t elements = requiredElements(_size);
		_buffer = (Type *)core_malloc(elements * sizeof(Type));
		core_memcpy(_buffer, other._buffer, elements * sizeof(Type));
	}
}

DynamicBitSet &DynamicBitSet::operator=(const DynamicBitSet &other) {
	if (this != &other) {
		core_free(_buffer);
		_buffer = nullptr;
		_size = other._size;
		if (_size > 0) {
			const size_t elements = requiredElements(_size);
			_buffer = (Type *)core_malloc(elements * sizeof(Type));
			core_memcpy(_buffer, other._buffer, elements * sizeof(Type));
		}
	}
	return *this;
}

DynamicBitSet::DynamicBitSet(DynamicBitSet &&other) noexcept : _buffer(other._buffer), _size(other._size) {
	other._buffer = nullptr;
	other._size = 0;
}

DynamicBitSet &DynamicBitSet::operator=(DynamicBitSet &&other) noexcept {
	if (this != &other) {
		core_free(_buffer);
		_buffer = other._buffer;
		_size = other._size;
		other._buffer = nullptr;
		other._size = 0;
	}
	return *this;
}

void DynamicBitSet::resize(size_t newSize) {
	if (newSize == _size) {
		return;
	}
	if (newSize > 0) {
		const size_t newElements = requiredElements(newSize);
		const size_t elements = requiredElements(_size);
		Type *newBuffer = (Type *)core_malloc(newElements * sizeof(Type));
		const size_t toCopy = core_min(elements, newElements);
		core_memcpy(newBuffer, _buffer, toCopy * sizeof(Type));
		for (size_t i = toCopy; i < newElements; ++i) {
			newBuffer[i] = Type(0);
		}
		_buffer = newBuffer;
		_size = newSize;
		return;
	}

	core_free(_buffer);
	_buffer = nullptr;
	_size = newSize;
}

void DynamicBitSet::fill() {
	const size_t elements = requiredElements(_size);
	for (size_t i = 0; i < elements; ++i) {
		_buffer[i] = ~Type(0);
	}
}

void DynamicBitSet::clear() {
	const size_t elements = requiredElements(_size);
	for (size_t i = 0; i < elements; ++i) {
		_buffer[i] = Type(0);
	}
}

size_t DynamicBitSet::bytes() const {
	return requiredElements(_size) * sizeof(Type);
}

bool DynamicBitSet::operator[](size_t idx) const {
	if (idx >= _size) {
		return false;
	}
	const size_t arrayIdx = idx / bitsPerValue;
	const size_t elementIdx = idx % bitsPerValue;
	const Type &ref = _buffer[arrayIdx];
	const Type mask = (Type(1) << elementIdx);
	const bool val = (ref & mask) != 0;
	return val;
}

void DynamicBitSet::set(size_t idx, bool value) {
	if (idx >= _size) {
		return;
	}
	const size_t arrayIdx = idx / bitsPerValue;
	const size_t elementIdx = idx % bitsPerValue;
	Type &ref = _buffer[arrayIdx];
	if (value) {
		ref |= (Type(1) << elementIdx);
	} else {
		ref &= ~(Type(1) << elementIdx);
	}
}

void DynamicBitSet::invert() {
	const size_t elements = requiredElements(_size);
	for (size_t i = 0; i < elements; ++i) {
		_buffer[i] = ~_buffer[i];
	}
}

bool DynamicBitSet::operator==(const DynamicBitSet &other) const {
	if (_size != other._size) {
		return false;
	}
	const size_t elements = requiredElements(_size);
	for (size_t i = 0; i < elements; ++i) {
		if (_buffer[i] != other._buffer[i]) {
			return false;
		}
	}
	return true;
}

bool DynamicBitSet::operator!=(const DynamicBitSet &other) const {
	return !(*this == other);
}

} // namespace core
