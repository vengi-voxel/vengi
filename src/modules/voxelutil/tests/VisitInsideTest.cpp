/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/Region.h"
#include "voxelutil/VisitInside.h"

namespace voxelutil {

class VisitInsideTest : public app::AbstractTest {};

TEST_F(VisitInsideTest, testInvalidRegionVisitsNothing) {
	const int n = visitInside(voxel::Region::InvalidRegion, false, [](int, int, int) { return true; },
							  [](int, int, int) {});
	EXPECT_EQ(0, n);
}

TEST_F(VisitInsideTest, testSolidBox) {
	const voxel::Region region(0, 2);
	const int n = visitInside(region, false, [](int, int, int) { return true; }, [](int, int, int) {});
	EXPECT_EQ(region.voxels(), (int64_t)n);
}

TEST_F(VisitInsideTest, testShellDropsInterior) {
	const voxel::Region region(0, 2);
	bool sawCenter = false;
	const int n = visitInside(
		region, true, [](int, int, int) { return true; },
		[&](int x, int y, int z) { sawCenter = sawCenter || (x == 1 && y == 1 && z == 1); });
	EXPECT_EQ(region.voxels() - 1, (int64_t)n);
	EXPECT_FALSE(sawCenter);
}

TEST_F(VisitInsideTest, testPredicateLimitsTheBox) {
	const voxel::Region region(0, 4);
	auto inner = [](int x, int y, int z) {
		return x >= 1 && x <= 3 && y >= 1 && y <= 3 && z >= 1 && z <= 3;
	};
	const int solid = visitInside(region, false, inner, [](int, int, int) {});
	bool sawCenter = false;
	const int shell = visitInside(region, true, inner, [&](int x, int y, int z) {
		sawCenter = sawCenter || (x == 2 && y == 2 && z == 2);
	});
	EXPECT_EQ(27, solid);
	EXPECT_EQ(26, shell);
	EXPECT_FALSE(sawCenter);
}

} // namespace voxelutil
