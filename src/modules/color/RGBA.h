/**
 * @file
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace color {

union alignas(4) RGBA {
	constexpr RGBA(uint32_t val = 0) : rgba(val) {
	}
	constexpr RGBA(uint8_t valr, uint8_t valg, uint8_t valb, uint8_t vala = 255) : r(valr), g(valg), b(valb), a(vala) {
	}
	struct {
		uint8_t r, g, b, a;
	};
	uint32_t rgba;

	inline operator uint32_t() const {
		return rgba;
	}

	inline RGBA &operator=(uint32_t other) {
		rgba = other;
		return *this;
	}

	inline bool operator==(RGBA other) const {
		return rgba == other.rgba;
	}

	inline bool operator!=(RGBA other) const {
		return rgba != other.rgba;
	}

	inline bool operator==(uint32_t other) const {
		return rgba == other;
	}

	inline constexpr uint8_t operator[](size_t index) const {
		switch (index) {
		case 0:
			return r;
		case 1:
			return g;
		case 2:
			return b;
		case 3:
			return a;
		default:
			return 0;
		}
	}

	inline bool operator!=(uint32_t other) const {
		return rgba != other;
	}

	static color::RGBA mix(const color::RGBA rgba1, const color::RGBA rgba2, float t = 0.5f);

	constexpr double brightness() const {
		return 0.299 * (double)r + 0.587 * (double)g + 0.114 * (double)b;
	}
};

static_assert(sizeof(RGBA) == sizeof(uint32_t), "Expected RGBA union size");

struct RGBAHasher {
	inline size_t operator()(const RGBA &o) const {
		return (size_t)o.rgba;
	}
};

} // namespace color
