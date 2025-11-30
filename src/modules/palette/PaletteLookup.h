/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/collection/Buffer.h"
#include <glm/fwd.hpp>

namespace palette {

class Palette;

/**
 * @brief A lookup table for palette colors, allowing fast retrieval of the closest color index
 * from a given RGBA color value.
 *
 * This class uses a LUT to store the mapping between RGBA colors and their corresponding
 * palette indices, enabling efficient lookups based on quantization - which basically means that
 * there is a loss of precision when mapping colors to palette indices - but this is a trade-off
 * for speed. The LUT is designed to cover a wide range of colors, but it may not be exhaustive.
 *
 * @note Thread-safe implementation for concurrent access.
 */
class PaletteLookup {
private:
	const palette::Palette &_palette;
	core::Buffer<uint16_t> _cache; // Fixed-size lookup

public:
	PaletteLookup(const palette::Palette &palette);

	inline const palette::Palette &palette() const {
		return _palette;
	}

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @param color Normalized color value [0.0-1.0]
	 * @sa color::Color::getClosestMatch()
	 */
	uint8_t findClosestIndex(const glm::vec4 &color);

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @sa color::Color::getClosestMatch()
	 */
	uint8_t findClosestIndex(color::RGBA rgba);
};

} // namespace palette
