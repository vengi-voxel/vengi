/**
 * @file
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace core {

union RGBA {
	constexpr RGBA(uint32_t val = 0) : rgba(val) {
	}
	constexpr RGBA(uint8_t valr, uint8_t valg, uint8_t valb, uint8_t vala = 255)
		: r(valr), g(valg), b(valb), a(vala) {
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

	inline bool operator!=(uint32_t other) const {
		return rgba != other;
	}

	static core::RGBA mix(const core::RGBA rgba1, const core::RGBA rgba2, float t = 0.5f);
};

static_assert(sizeof(RGBA) == sizeof(uint32_t), "Expected RGBA union size");

struct RGBAHasher {
	inline size_t operator() (const RGBA& o) const {
		return (size_t)o.rgba;
	}
};

} // namespace core
