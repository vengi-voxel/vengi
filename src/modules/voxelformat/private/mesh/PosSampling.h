/**
 * @file
 */

#include "core/RGBA.h"
#include "core/collection/Array.h"

#define MaxTriangleColorContributions 4
#define AlphaThreshold 0

namespace voxelformat {

/**
 * @brief Weighted color entry for a position for averaging the voxel color value over all positions that were found
 * during voxelization
 */
struct PosSamplingEntry {
	inline PosSamplingEntry(uint32_t _area, core::RGBA _color, uint8_t _normal)
		: area(_area), normal(_normal), color(_color) {
	}
	PosSamplingEntry() : area(0) {
	}
	uint32_t area : 24;
	uint8_t normal = 0u;
	core::RGBA color;
};

/**
 * @brief Weighted color entry for a position for averaging the voxel color value over all positions that were found
 * during voxelization
 */
class PosSampling {
private:
	core::Array<PosSamplingEntry, MaxTriangleColorContributions> entries;

public:
	PosSampling(uint32_t area, core::RGBA color, uint8_t normal) {
		entries[0].area = area;
		entries[0].color = color;
		entries[0].normal = normal;
	}

	bool add(uint32_t area, core::RGBA color, uint8_t normal);
	/**
	 * @brief Computes the color based on the position sampling entries.
	 *
	 * This function calculates the color from the position sampling entries. If there is only one entry,
	 * it returns the flattened color of that entry. If there are multiple entries, it can either compute
	 * a weighted average of the colors based on the area of each entry or return the color of the entry
	 * with the largest area.
	 *
	 * @param flattenFactor The factor used to flatten the RGB values of the resulting color.
	 * @param weightedAverage If @c true, the function computes a weighted average of the colors based on the area of
	 * each entry. If @c false, the function returns the color of the entry with the largest area.
	 *
	 * @sa core::Color::flattenRGB()
	 *
	 * @return The computed color as a core::RGBA value.
	 */
	core::RGBA getColor(uint8_t flattenFactor, bool weightedAverage) const;
	uint8_t getNormal() const;
};

} // namespace voxelformat
