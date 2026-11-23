/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "palette/NormalPalette.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace palette {

/**
 * @brief Precomputing a lookup table with octahedral encoding
 * @note keep this cached, the initial ramp-up is expensive
 *
 * @note Accuracy Considerations for TABLE_RES = 32:
 * - The lookup grid becomes 32x32 = 1024 cells covering the unit sphere via octahedral mapping.
 * - Each cell represents a small angular region (~5.6 degrees across), providing much finer approximation
 *   than 16x16 (~11.25 degrees).
 * - For a palette of 256 normals, this means each grid cell is significantly smaller than the
 *   average angular spacing between normals, minimizing mismatches.
 * - Memory cost is still minimal (1024 entries ~= 4 KB), so 32x32 is a good balance between
 *   speed, accuracy, and memory for most rendering or shading use cases.
 */
class NormalPaletteLookup {
private:
	static const int TABLE_RES = 32;
	core::Array<int, TABLE_RES * TABLE_RES> _lookupTable;

	math::Vec3f octDecode(const math::Vec2f &e) const {
		const math::Vec2f f = e * 2.0f - 1.0f;
		math::Vec3f n(f.x, f.y, 1.0f - math::abs(f.x) - math::abs(f.y));
		const float t = math::clamp(-n.z, 0.0f, 1.0f);
		n.x += (n.x >= 0.0f) ? -t : t;
		n.y += (n.y >= 0.0f) ? -t : t;
		return math::normalize(n);
	}

	math::Vec2f octEncode(const math::Vec3f &n) const {
		const math::Vec3f absN = math::abs(n);
		const float invL1 = 1.0f / (absN.x + absN.y + absN.z);
		math::Vec2f res(n.x * invL1, n.y * invL1);
		if (n.z < 0.0f) {
			res = (1.0f - math::abs(math::Vec2f(res.y, res.x))) *
				  math::Vec2f((res.x >= 0.0f) ? 1.0f : -1.0f, (res.y >= 0.0f) ? 1.0f : -1.0f);
		}
		return (res * 0.5f + 0.5f); // Map from [-1,1] to [0,1]
	}

public:
	NormalPaletteLookup(const palette::NormalPalette &palette) {
		math::Vec3f normals[NormalPaletteMaxNormals];
		for (size_t i = 0; i < palette.size(); ++i) {
			normals[i] = palette.normal3f(i);
		}

		for (int y = 0; y < TABLE_RES; ++y) {
			for (int x = 0; x < TABLE_RES; ++x) {
				// Decode back to normal from grid center
				const math::Vec2f uv((x + 0.5f) / TABLE_RES, (y + 0.5f) / TABLE_RES);
				const math::Vec3f normal = octDecode(uv);

				// Find closest palette normal
				int bestIndex = PaletteNormalNotFound;
				float maxDot = -2.0f;
				for (size_t i = 0; i < palette.size(); ++i) {
					if (palette.normal(i).a == 0) {
						continue;
					}
					const float dot = math::dot(normal, normals[i]);
					if (dot > maxDot) {
						maxDot = dot;
						bestIndex = i;
					}
				}

				_lookupTable[y * TABLE_RES + x] = bestIndex;
			}
		}
	}

	int getClosestMatch(const math::Vec3f &normal) const {
		const math::Vec2f &encoded = octEncode(normal);
		const int x = math::clamp(int(encoded.x * TABLE_RES), 0, TABLE_RES - 1);
		const int y = math::clamp(int(encoded.y * TABLE_RES), 0, TABLE_RES - 1);
		return _lookupTable[y * TABLE_RES + x];
	}
};

} // namespace palette
