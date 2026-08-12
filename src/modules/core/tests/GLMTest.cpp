/**
 * @file
 */

#include "core/GLM.h"
#include <gtest/gtest.h>

namespace glm {

TEST(GLMTest, testIntersectTriangleAABBUnitVoxelHit) {
	const vec3 half(0.5f);
	const vec3 center(0.0f);
	/* Triangle covering the unit voxel around the origin */
	const vec3 v0(-1.0f, -1.0f, 0.0f);
	const vec3 v1(1.0f, -1.0f, 0.0f);
	const vec3 v2(0.0f, 1.0f, 0.0f);
	EXPECT_TRUE(intersectTriangleAABB(center, half, v0, v1, v2));

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);
	EXPECT_TRUE(intersectTriangleAABB(center, prep, v0, v1, v2));
}

TEST(GLMTest, testIntersectTriangleAABBUnitVoxelMiss) {
	const vec3 half(0.5f);
	const vec3 center(10.0f, 10.0f, 10.0f);
	const vec3 v0(-1.0f, -1.0f, 0.0f);
	const vec3 v1(1.0f, -1.0f, 0.0f);
	const vec3 v2(0.0f, 1.0f, 0.0f);
	EXPECT_FALSE(intersectTriangleAABB(center, half, v0, v1, v2));

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);
	EXPECT_FALSE(intersectTriangleAABB(center, prep, v0, v1, v2));
}

TEST(GLMTest, testIntersectTriangleAABBPrepMatchesDirect) {
	const vec3 half(0.5f);
	const vec3 v0(0.1f, 0.2f, -0.3f);
	const vec3 v1(2.5f, -0.4f, 1.1f);
	const vec3 v2(-1.2f, 3.0f, 0.7f);

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);

	for (int x = -2; x <= 4; ++x) {
		for (int y = -2; y <= 4; ++y) {
			for (int z = -2; z <= 4; ++z) {
				const vec3 center((float)x, (float)y, (float)z);
				const bool direct = intersectTriangleAABB(center, half, v0, v1, v2);
				const bool prepared = intersectTriangleAABB(center, prep, v0, v1, v2);
				EXPECT_EQ(direct, prepared) << "center=" << x << "," << y << "," << z;
			}
		}
	}
}

TEST(GLMTest, testIntersectTriangleAABBPrepMatchesDirectSlanted) {
	const vec3 half(0.5f);
	/* Large slanted triangle spanning many voxels */
	const vec3 v0(0.0f, 0.0f, 0.0f);
	const vec3 v1(8.0f, 0.0f, 2.0f);
	const vec3 v2(0.0f, 8.0f, 4.0f);

	TriangleAABBPrep prep;
	prepareTriangleAABB(v0, v1, v2, half, prep);

	int hits = 0;
	for (int x = -1; x <= 9; ++x) {
		for (int y = -1; y <= 9; ++y) {
			for (int z = -1; z <= 5; ++z) {
				const vec3 center((float)x, (float)y, (float)z);
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

} // namespace glm
