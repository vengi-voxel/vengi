/**
 * @file
 */

#include "core/Assert.h"
#include "core/StandardLib.h"
#include <limits.h>
#include <stdint.h>

namespace core {

/**
 * @brief Allows to store boolean values in a compact bit buffer
 * @ingroup Collections
 */
class BitSet {
private:
	using Type = uint32_t;
	int _bits;
	size_t _bytes;
	Type *_buffer = nullptr;
	static constexpr size_t bitsPerValue = sizeof(Type) * CHAR_BIT;
	static_assert(bitsPerValue == 32, "Unexpected size");

	static inline constexpr size_t align(size_t bits) {
		const size_t bytes = bits % CHAR_BIT == 0 ? bits / CHAR_BIT : bits / CHAR_BIT + 1;
		const size_t len = 31u;
		const size_t aligned = ((bytes + len) & ~len);
		return aligned;
	}

public:
	BitSet(int bits) : _bits(bits) {
		_bytes = align(_bits);
		_buffer = (Type *)core_malloc(_bytes);
		clear();
	}

	~BitSet() {
		core_free(_buffer);
	}

	inline int bits() const {
		return _bits;
	}

	inline void fill() {
		core_memset(_buffer, 0xFFu, _bytes);
	}

	inline void clear() {
		core_memset(_buffer, 0x00u, _bytes);
	}

	inline size_t bytes() const {
		return _bytes;
	}

	inline bool operator[](size_t idx) const {
		core_assert_msg((int)idx < _bits, "index out of bounds %i", (int)idx);
		const size_t arrayIdx = idx / bitsPerValue;
		const size_t elementIdx = idx % bitsPerValue;
		const Type &ref = _buffer[arrayIdx];
		const Type mask = (Type(1) << elementIdx);
		const bool val = (ref & mask) != 0;
		return val;
	}

	inline void set(size_t idx, bool value) const {
		core_assert_msg((int)idx < _bits, "index out of bounds %i", (int)idx);
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
