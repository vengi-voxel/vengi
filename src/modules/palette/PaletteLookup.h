/**
 * @file
 */

#pragma once

#include "core/Algorithm.h"
#include "core/Color.h"
#include "core/collection/DynamicArray.h"
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
	palette::Palette _palette;
	core::DynamicMap<core::RGBA, uint8_t, 521> _paletteMap;

	struct KDPoint {
		core::RGBA color;
		uint8_t index;

		bool operator<(const KDPoint &other) const {
			return color.r < other.color.r;
		}
	};
	core::DynamicArray<KDPoint> _points;

	static int dimension(int depth) {
		return depth % 4;
	}

	static float distanceSq(core::RGBA a, core::RGBA b) {
		const float dr = (float)a.r - (float)b.r;
		const float dg = (float)a.g - (float)b.g;
		const float db = (float)a.b - (float)b.b;
		const float da = (float)a.a - (float)b.a;

		return dr * dr + dg * dg + db * db + da * da;
	}

	void buildRecursive(size_t start, size_t end, int depth) {
		if (start >= end)
			return;

		const size_t mid = start + (end - start) / 2;
		const int dim = dimension(depth);
		core::nth_element(_points, start, end, mid,
						  [dim](const KDPoint &a, const KDPoint &b) { return a.color[dim] < b.color[dim]; });
		if (start < mid) {
			buildRecursive(start, mid, depth + 1);
		}
		if (mid + 1 < end) {
			buildRecursive(mid + 1, end, depth + 1);
		}
	}

	void nearestRecursive(size_t start, size_t end, int depth, core::RGBA color, float &bestDistance,
						  uint8_t &bestIndex) const {
		if (start >= end)
			return;

		const size_t mid = start + (end - start) / 2;
		const float dist = distanceSq(color, _points[mid].color);
		if (dist < bestDistance) {
			bestDistance = dist;
			bestIndex = _points[mid].index;
		}

		const int dim = dimension(depth);
		const float diff = color[dim] - _points[mid].color[dim];
		const float diffSq = diff * diff;
		const size_t first = (diff < 0) ? start : mid + 1;
		const size_t second = (diff < 0) ? mid + 1 : start;

		nearestRecursive(first, (diff < 0) ? mid : end, depth + 1, color, bestDistance, bestIndex);

		if (diffSq < bestDistance) {
			nearestRecursive(second, (diff < 0) ? end : mid, depth + 1, color, bestDistance, bestIndex);
		}
	}

	void initialize() {
		_points.reserve(_palette.colorCount()); // Allocate just once
		for (int i = 0; i < _palette.colorCount(); ++i) {
			_points.push_back({_palette.color(i), (uint8_t)i});
		}
		buildRecursive(0, _points.size(), 0);
	}
public:
	PaletteLookup(const palette::Palette &palette) : _palette(palette) {
		if (_palette.colorCount() <= 0) {
			_palette.nippon();
		}
		initialize();
	}

	PaletteLookup() {
		_palette.nippon();
		initialize();
	}

	inline const palette::Palette &palette() const {
		return _palette;
	}

	inline palette::Palette &palette() {
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
			float bestDistance = FLT_MAX;
			nearestRecursive(0, _points.size(), 0, rgba, bestDistance, paletteIndex);
			_paletteMap.put(rgba, paletteIndex);
		}
		return paletteIndex;
	}
};

} // namespace palette
