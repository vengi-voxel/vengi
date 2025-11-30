/**
 * @file
 */

#pragma once

#include "MeshMaterial.h"
#include "color/RGBA.h"
#include "core/collection/Array.h"

#define MaxTriangleColorContributions 4
#define AlphaThreshold 0

namespace voxelformat {

/**
 * @brief Weighted color entry for a position for averaging the voxel color value over all positions that were found
 * during voxelization
 */
struct PosSamplingEntry {
	inline PosSamplingEntry(uint32_t _area, color::RGBA _color, uint8_t _normal, MeshMaterialIndex _materialIdx)
		: area(_area), color(_color), normal(_normal), materialIdx(_materialIdx) {
	}
	PosSamplingEntry() = default;
	uint32_t area = 0u;
	color::RGBA color;
	uint8_t normal = 0u;
	MeshMaterialIndex materialIdx = 0u;
};
// static_assert(sizeof(PosSamplingEntry) == 12, "");

/**
 * @brief Weighted color entry for a position for averaging the voxel color value over all positions that were found
 * during voxelization
 */
class PosSampling {
private:
	core::Array<PosSamplingEntry, MaxTriangleColorContributions> entries;

public:
	PosSampling(uint32_t area, color::RGBA color, uint8_t normal, MeshMaterialIndex materialIdx) {
		entries[0].area = area;
		entries[0].color = color;
		entries[0].normal = normal;
		entries[0].materialIdx = materialIdx;
	}

	bool add(uint32_t area, color::RGBA color, uint8_t normal, MeshMaterialIndex materialIdx);
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
	 * @sa color::flattenRGB()
	 *
	 * @return The computed color as a color::RGBA value.
	 */
	color::RGBA getColor(uint8_t flattenFactor, bool weightedAverage) const;
	uint8_t getNormal() const;
	MeshMaterialIndex getMaterialIndex() const;
};

} // namespace voxelformat
