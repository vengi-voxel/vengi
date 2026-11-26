/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicMap.h"
#include "math/Vec4f.h"

namespace palette {

class Palette;

/**
 * @brief A lookup table for palette colors, allowing fast retrieval of the closest color index
 * from a given RGBA color value.
 *
 * Approximate precision caches a match per quantized RGBA value.
 * Exact precision preserves distinct palette colors and computes the closest match for other colors.
 * @note Thread-safe implementation for concurrent access.
 */
class PaletteLookup {
public:
	enum class Precision {
		Approximate,
		Exact
	};

private:
	const palette::Palette &_palette;
	core::Buffer<uint16_t> _cache;
	core::DynamicMap<color::RGBA, uint8_t, 257, color::RGBAHasher> _exactMatches;
	Precision _precision;

public:
	/** Use exact precision when the source colors also define the palette. */
	explicit PaletteLookup(const palette::Palette &palette, Precision precision = Precision::Approximate);

	inline const palette::Palette &palette() const {
		return _palette;
	}

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @param color Normalized color value [0.0-1.0]
	 * @sa color::getClosestMatch()
	 */
	uint8_t findClosestIndex(const math::Vec4f &color);

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @sa color::getClosestMatch()
	 */
	uint8_t findClosestIndex(color::RGBA rgba);
};

} // namespace palette
