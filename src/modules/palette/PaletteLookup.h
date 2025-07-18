/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Color.h"
#include "core/collection/DynamicMap.h"
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
	const palette::Palette &_palette;
	// TODO: PERF: use a k-d tree, octree, ANN or whatever to prevent the O(n) search
	// for the closest color match
	core::DynamicMap<core::RGBA, uint8_t, 521> _paletteMap;

public:
	PaletteLookup(const palette::Palette &palette) : _palette(palette) {
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
		uint8_t paletteIndex = 0;
		if (!_paletteMap.get(rgba, paletteIndex)) {
			core_assert_always(_palette.colorCount() > 0);
			// here we are O(n) in every case where the colors are not a perfect match
			paletteIndex = _palette.getClosestMatch(rgba);
			_paletteMap.put(rgba, paletteIndex);
		}
		return paletteIndex;
	}
};

} // namespace palette
