/**
 * @file
 */

#pragma once

#include <limits.h>
#include <stdint.h>

namespace core {

/**
 * @brief Allows to store boolean values in a compact bit buffer
 * @ingroup Collections
 */
template<size_t SIZE>
class BitSet {
private:
	static_assert(SIZE > 0, "BitSet size must be greater than 0");
	using Type = uint32_t;
	static constexpr size_t requiredElements(size_t bits) {
		const int bitsPerType = sizeof(Type) * CHAR_BIT;
		return bits % bitsPerType == 0 ? bits / bitsPerType : bits / bitsPerType + 1;
	}
	Type _buffer[requiredElements(SIZE)];
	static constexpr size_t bitsPerValue = sizeof(Type) * CHAR_BIT;
public:
	BitSet() {
		clear();
	}

	inline constexpr int bits() const {
		return SIZE;
	}

	inline constexpr void fill() {
		for (size_t i = 0; i < requiredElements(SIZE); ++i) {
			_buffer[i] = ~Type(0);
		}
	}

	inline void clear() {
		for (size_t i = 0; i < requiredElements(SIZE); ++i) {
			_buffer[i] = Type(0);
		}
	}

	inline constexpr size_t bytes() const {
		return sizeof(_buffer);
	}

	inline constexpr bool operator[](size_t idx) const {
		if (idx >= SIZE) {
			return false;
		}
		const size_t arrayIdx = idx / bitsPerValue;
		const size_t elementIdx = idx % bitsPerValue;
		const Type &ref = _buffer[arrayIdx];
		const Type mask = (Type(1) << elementIdx);
		const bool val = (ref & mask) != 0;
		return val;
	}

	inline constexpr void set(size_t idx, bool value) {
		if (idx >= SIZE) {
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
};

} // namespace core
