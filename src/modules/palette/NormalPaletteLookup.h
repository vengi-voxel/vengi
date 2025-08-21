/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "palette/NormalPalette.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace palette {

/**
 * @brief Precomputing a lookup table with octahedral encoding
 * @note keep this cached, the initial ramp-up is expensive
 *
 * @note Accuracy Considerations for TABLE_RES = 32:
 * - The lookup grid becomes 32x32 = 1024 cells covering the unit sphere via octahedral mapping.
 * - Each cell represents a small angular region (~5.6° across), providing much finer approximation
 *   than 16x16 (~11.25°).
 * - For a palette of 256 normals, this means each grid cell is significantly smaller than the
 *   average angular spacing between normals, minimizing mismatches.
 * - Memory cost is still minimal (1024 entries ≈ 4 KB), so 32x32 is a good balance between
 *   speed, accuracy, and memory for most rendering or shading use cases.
 */
class NormalPaletteLookup {
private:
	static const int TABLE_RES = 32;
	core::Array<int, TABLE_RES * TABLE_RES> _lookupTable;

	glm::vec3 octDecode(const glm::vec2 &e) const {
		const glm::vec2 f = e * 2.0f - 1.0f;
		glm::vec3 n(f.x, f.y, 1.0f - glm::abs(f.x) - glm::abs(f.y));
		const float t = glm::clamp(-n.z, 0.0f, 1.0f);
		n.x += (n.x >= 0.0f) ? -t : t;
		n.y += (n.y >= 0.0f) ? -t : t;
		return glm::normalize(n);
	}

	glm::vec2 octEncode(const glm::vec3 &n) const {
		const glm::vec3 absN = glm::abs(n);
		const float invL1 = 1.0f / (absN.x + absN.y + absN.z);
		glm::vec2 res(n.x * invL1, n.y * invL1);
		if (n.z < 0.0f) {
			res = (1.0f - glm::abs(glm::vec2(res.y, res.x))) *
				  glm::vec2((res.x >= 0.0f) ? 1.0f : -1.0f, (res.y >= 0.0f) ? 1.0f : -1.0f);
		}
		return (res * 0.5f + 0.5f); // Map from [-1,1] to [0,1]
	}

public:
	NormalPaletteLookup(const palette::NormalPalette &palette) {
		glm::vec3 normals[NormalPaletteMaxNormals];
		for (size_t i = 0; i < palette.size(); ++i) {
			normals[i] = palette.normal3f(i);
		}

		for (int y = 0; y < TABLE_RES; ++y) {
			for (int x = 0; x < TABLE_RES; ++x) {
				// Decode back to normal from grid center
				const glm::vec2 uv((x + 0.5f) / TABLE_RES, (y + 0.5f) / TABLE_RES);
				const glm::vec3 normal = octDecode(uv);

				// Find closest palette normal
				int bestIndex = PaletteNormalNotFound;
				float maxDot = -1.0f;
				for (size_t i = 0; i < palette.size(); ++i) {
					const float dot = glm::dot(normal, normals[i]);
					if (dot > maxDot) {
						maxDot = dot;
						bestIndex = i;
					}
				}

				_lookupTable[y * TABLE_RES + x] = bestIndex;
			}
		}
	}

	int getClosestMatch(const glm::vec3 &normal) const {
		const glm::vec2 &encoded = octEncode(normal);
		const int x = glm::clamp(int(encoded.x * TABLE_RES), 0, TABLE_RES - 1);
		const int y = glm::clamp(int(encoded.y * TABLE_RES), 0, TABLE_RES - 1);
		return _lookupTable[y * TABLE_RES + x];
	}
};

} // namespace palette
