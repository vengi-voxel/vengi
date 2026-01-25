/**
 * @file
 */

#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

namespace core {

/**
 * @brief Allows to store boolean values in a compact bit buffer with runtime-determined size
 * @ingroup Collections
 */
class DynamicBitSet {
public:
	using Type = uint32_t;

private:
	static constexpr size_t bitsPerValue = sizeof(Type) * CHAR_BIT;

	static size_t requiredElements(size_t bits) {
		return bits == 0 ? 0 : (bits + bitsPerValue - 1) / bitsPerValue;
	}

	Type *_buffer = nullptr;
	size_t _size = 0; // number of bits

public:
	DynamicBitSet() = default;

	explicit DynamicBitSet(size_t size);
	~DynamicBitSet();
	DynamicBitSet(const DynamicBitSet &other);
	DynamicBitSet &operator=(const DynamicBitSet &other);
	DynamicBitSet(DynamicBitSet &&other) noexcept;
	DynamicBitSet &operator=(DynamicBitSet &&other) noexcept;

	inline size_t bits() const {
		return _size;
	}

	void resize(size_t newSize);

	void fill();
	void invert();

	void clear();

	size_t bytes() const;

	void set(size_t idx, bool value);

	bool operator[](size_t idx) const;
	bool operator==(const DynamicBitSet &other) const;
	bool operator!=(const DynamicBitSet &other) const;
};

} // namespace core
