/**
 * @file
 */

#include "math/TriangleAABB.h"
#include <gtest/gtest.h>

namespace math {

TEST(TriangleAABB, testIntersectTriangleAABBUnitVoxelHit) {
	const Vec3f half(0.5f);
	const Vec3f center(0.0f);
	/* Triangle covering the unit voxel around the origin */
	const Vec3f v0(-1.0f, -1.0f, 0.0f);
	const Vec3f v1(1.0f, -1.0f, 0.0f);
	const Vec3f v2(0.0f, 1.0f, 0.0f);
	EXPECT_TRUE(intersectTriangleAABB(center, half, v0, v1, v2));

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);
	EXPECT_TRUE(intersectTriangleAABB(center, prep, v0, v1, v2));
}

TEST(TriangleAABB, testIntersectTriangleAABBUnitVoxelMiss) {
	const Vec3f half(0.5f);
	const Vec3f center(10.0f, 10.0f, 10.0f);
	const Vec3f v0(-1.0f, -1.0f, 0.0f);
	const Vec3f v1(1.0f, -1.0f, 0.0f);
	const Vec3f v2(0.0f, 1.0f, 0.0f);
	EXPECT_FALSE(intersectTriangleAABB(center, half, v0, v1, v2));

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);
	EXPECT_FALSE(intersectTriangleAABB(center, prep, v0, v1, v2));
}

TEST(TriangleAABB, testIntersectTriangleAABBPrepMatchesDirect) {
	const Vec3f half(0.5f);
	const Vec3f v0(0.1f, 0.2f, -0.3f);
	const Vec3f v1(2.5f, -0.4f, 1.1f);
	const Vec3f v2(-1.2f, 3.0f, 0.7f);

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);

	for (int x = -2; x <= 4; ++x) {
		for (int y = -2; y <= 4; ++y) {
			for (int z = -2; z <= 4; ++z) {
				const Vec3f center((float)x, (float)y, (float)z);
				const bool direct = intersectTriangleAABB(center, half, v0, v1, v2);
				const bool prepared = intersectTriangleAABB(center, prep, v0, v1, v2);
				EXPECT_EQ(direct, prepared) << "center=" << x << "," << y << "," << z;
			}
		}
	}
}

TEST(TriangleAABB, testIntersectTriangleAABBPrepMatchesDirectSlanted) {
	const Vec3f half(0.5f);
	/* Large slanted triangle spanning many voxels */
	const Vec3f v0(0.0f, 0.0f, 0.0f);
	const Vec3f v1(8.0f, 0.0f, 2.0f);
	const Vec3f v2(0.0f, 8.0f, 4.0f);

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);

	int hits = 0;
	for (int x = -1; x <= 9; ++x) {
		for (int y = -1; y <= 9; ++y) {
			for (int z = -1; z <= 5; ++z) {
				const Vec3f center((float)x, (float)y, (float)z);
				const bool direct = intersectTriangleAABB(center, half, v0, v1, v2);
				const bool prepared = intersectTriangleAABB(center, prep, v0, v1, v2);
				EXPECT_EQ(direct, prepared) << "center=" << x << "," << y << "," << z;
				if (prepared) {
					++hits;
				}
			}
		}
	}
	EXPECT_GT(hits, 0);
}

} // namespace math
