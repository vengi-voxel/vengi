/**
 * @file
 */

#pragma once

#include "math/Vec3f.h"

namespace math {

/**
 * Calculate the line segment PaPb that is the shortest route between
 * two lines P1P2 and P3P4. Calculate also the values of @c mua and @c mub where
 *  Pa = P1 + mua (P2 - P1)
 *  Pb = P3 + mub (P4 - P3)
 * @return @c false if no solution exists.
 */
bool intersectLines(const math::Vec3f& p1,
		const math::Vec3f& p2, const math::Vec3f& p3, const math::Vec3f& p4,
		math::Vec3f& pa, math::Vec3f& pb, float *pmua = nullptr,
		float *pmub = nullptr);

/**
 * Precomputed, translation-invariant data for @c intersectTriangleAABB.
 * Edge vectors and separating-axis radii do not depend on the box center, so
 * callers that test one triangle against many AABBs (mesh voxelization) should
 * prepare once and reuse.
 */
struct TriangleAABBPrep {
	math::Vec3f e0;
	math::Vec3f e1;
	math::Vec3f e2;
	math::Vec3f normal;
	math::Vec3f halfSize;
	/* rad for AXISTEST_X01 / Y02 / Z12 (e0), X01 / Y02 / Z0 (e1), X2 / Y1 / Z12 (e2) */
	float radE0[3];
	float radE1[3];
	float radE2[3];
};

void prepareTriangleAABB(const math::Vec3f &v0, const math::Vec3f &v1, const math::Vec3f &v2, const math::Vec3f &halfSize,
						 TriangleAABBPrep &out);

bool intersectTriangleAABB(const math::Vec3f &center, const TriangleAABBPrep &prep, math::Vec3f v0, math::Vec3f v1,
						   math::Vec3f v2);

bool intersectTriangleAABB(const math::Vec3f &center, const math::Vec3f &halfSize, math::Vec3f v0, math::Vec3f v1,
						   math::Vec3f v2);

inline int intMod(int a, int b) {
	if (a > 0) {
		return a % b;
	}
	return (a % b + b) % b;
}

}
