/**
 * @file
 */

#include "MeshTri.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace voxelformat {

void MeshTri::setUVs(const math::Vec2f &uv1, const math::Vec2f &uv2, const math::Vec2f &uv3) {
	_uv[0] = uv1;
	_uv[1] = uv2;
	_uv[2] = uv3;
}

math::Vec2f MeshTri::centerUV() const {
	return (uv0() + uv1() + uv2()) / 3.0f;
}

int MeshTri::subdivideTriCount(size_t maxPerTriangle) const {
	const float maxSide = maxSideLength();
	if (maxSide <= 1.0f) {
		return 1;
	}
	int d = (int)math::ceil(math::log2(maxSide));
	if (d < 0) {
		d = 0;
	}
	if (d > 16) {
		d = 16; // match depth limit in subdivideTri
	}
	size_t trisCount = 1;
	for (int k = 0; k < d; ++k) {
		// multiply by 4 each level, but cap to avoid overflow
		if (trisCount > maxPerTriangle / 4) {
			trisCount = maxPerTriangle;
			break;
		}
		trisCount *= 4;
	}
	return trisCount;
}

float MeshTri::maxSideLength() const {
	const math::Vec3f &v0 = vertex0();
	const math::Vec3f &v1 = vertex1();
	const math::Vec3f &v2 = vertex2();
	const float s0 = math::length(v0 - v1);
	const float s1 = math::length(v1 - v2);
	const float s2 = math::length(v2 - v0);
	const float maxSide = math::max(math::max(s0, s1), s2);
	return maxSide;
}

// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
bool MeshTri::calcUVs(const math::Vec3f &pos, math::Vec2f &outUV) const {
	const math::Vec3f &b = calculateBarycentric(pos);

	// Check if barycentric coordinates are within [0, 1]
	if (b.x >= 0.0f && b.x <= 1.0f && b.y >= 0.0f && b.y <= 1.0f && b.z >= 0.0f && b.z <= 1.0f) {
		// Interpolate UVs using barycentric coordinates
		outUV = b.x * uv0() + b.y * uv1() + b.z * uv2();
		return true;
	}

	return false;
}

} // namespace voxelformat
