/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Color.h"
#include "core/collection/Buffer.h"
#include "palette/Palette.h"

namespace palette {

/**
 * @brief A lookup table for palette colors, allowing fast retrieval of the closest color index
 * from a given RGBA color value.
 *
 * This class uses a hash map to store the mapping between RGBA colors and their corresponding
 * palette indices, enabling efficient lookups.
 */
class PaletteLookup {
private:
	static constexpr int Q_BITS = 5;											 // 5 bits per channel
	static constexpr int Q_LEVELS = 1 << Q_BITS;								 // 32
	static constexpr int CACHE_SIZE = Q_LEVELS * Q_LEVELS * Q_LEVELS * Q_LEVELS; // 32^4 = 1048576

	const palette::Palette &_palette;
	core::Buffer<uint16_t> _cache; // Fixed-size lookup

	static inline uint16_t quantizeChannel(uint8_t value) {
		return value >> (8 - Q_BITS); // shift to keep top Q_BITS
	}

	static inline size_t computeIndex(core::RGBA rgba) {
		uint16_t r = quantizeChannel(rgba.r);
		uint16_t g = quantizeChannel(rgba.g);
		uint16_t b = quantizeChannel(rgba.b);
		uint16_t a = quantizeChannel(rgba.a);
		return ((r << (3 * Q_BITS)) | (g << (2 * Q_BITS)) | (b << Q_BITS) | a);
	}

public:
	PaletteLookup(const palette::Palette &palette) : _palette(palette) {
		_cache.resize(CACHE_SIZE);
		_cache.fill(PaletteColorNotFound);
	}

	inline const palette::Palette &palette() const {
		return _palette;
	}

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @param color Normalized color value [0.0-1.0]
	 * @sa core::Color::getClosestMatch()
	 */
	inline uint8_t findClosestIndex(const glm::vec4 &color) {
		return findClosestIndex(core::Color::getRGBA(color));
	}

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @sa core::Color::getClosestMatch()
	 */
	uint8_t findClosestIndex(core::RGBA rgba) {
		size_t idx = computeIndex(rgba);
		uint16_t &cached = _cache[idx];
		if (cached == (uint16_t)PaletteColorNotFound) {
			core_assert_always(_palette.colorCount() > 0);
			cached = _palette.getClosestMatch(rgba);
		}
		return cached;
	}
};

} // namespace palette
