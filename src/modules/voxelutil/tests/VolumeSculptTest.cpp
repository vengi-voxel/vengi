/**
 * @file
 */

#include "voxelutil/VolumeSculpt.h"
#include "app/tests/AbstractTest.h"
#include "voxel/BitVolume.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VolumeSculptTest : public app::AbstractTest {};

static int countSolid(const voxel::RawVolume &volume) {
	return visitVolumeParallel(volume, EmptyVisitor(), VisitSolid());
}

static void fillRegion(voxel::RawVolume &volume, const voxel::Region &region, const voxel::Voxel &voxel) {
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				volume.setVoxel(x, y, z, voxel);
			}
		}
	}
}

TEST_F(VolumeSculptTest, testErodeRemovesProtrusion) {
	// 3x3x3 cube with a single protrusion voxel on top
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	fillRegion(volume, voxel::Region(1, 3), solid);
	// Add a protrusion: single voxel sticking out
	volume.setVoxel(2, 4, 2, solid);

	const int before = countSolid(volume);
	const int changed = sculptErode(volume, region, 0.5f, 1);
	EXPECT_GT(changed, 0);
	EXPECT_LT(countSolid(volume), before);
	// The protrusion (1 face-neighbor) should be removed at threshold 2
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 4, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testErodeStrengthZeroNoChange) {
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);

	const int before = countSolid(volume);
	sculptErode(volume, region, 0.0f, 1);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testGrowFillsConcavity) {
	// L-shaped region: two arms of a cube missing a corner
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Fill a 3x3x3 cube but remove a corner to create a concavity
	fillRegion(volume, voxel::Region(1, 3), solid);
	volume.setVoxel(3, 3, 3, voxel::Voxel());

	const int before = countSolid(volume);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	sculptGrow(volume, region, 0.7f, 1, fill);
	// The concavity (3 face-neighbors) should be filled at threshold ~3
	EXPECT_GE(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testGrowStrengthZeroNoChange) {
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);
	volume.setVoxel(3, 3, 3, voxel::Voxel());

	const int before = countSolid(volume);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	sculptGrow(volume, region, 0.0f, 1, fill);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testFlattenPositiveY) {
	// 3x3x3 cube, flatten from top (PositiveY)
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);

	const int before = countSolid(volume);
	const int changed = sculptFlatten(volume, region, voxel::FaceNames::PositiveY, 1);
	EXPECT_GT(changed, 0);
	EXPECT_LT(countSolid(volume), before);
	// Top layer (y=3) should be removed
	for (int z = 1; z <= 3; ++z) {
		for (int x = 1; x <= 3; ++x) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 3, z).getMaterial()));
		}
	}
	// Layer below (y=2) should still exist
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testFlattenNegativeX) {
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);

	sculptFlatten(volume, region, voxel::FaceNames::NegativeX, 1);
	// Lowest X layer (x=1) should be removed
	for (int z = 1; z <= 3; ++z) {
		for (int y = 1; y <= 3; ++y) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(1, y, z).getMaterial()));
		}
	}
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testFlattenMultipleIterations) {
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);

	sculptFlatten(volume, region, voxel::FaceNames::PositiveY, 2);
	// Both y=3 and y=2 layers should be removed
	for (int z = 1; z <= 3; ++z) {
		for (int x = 1; x <= 3; ++x) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 3, z).getMaterial()));
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 2, z).getMaterial()));
		}
	}
	// y=1 should remain
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 1, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testAnchorsPreventErosion) {
	// A single voxel in the sculpt region with a solid anchor neighbor outside
	// should have higher neighbor count and resist erosion
	voxel::Region fullRegion(0, 4);
	voxel::RawVolume volume(fullRegion);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Place a line of voxels at y=2, x=1..3, z=2
	for (int x = 1; x <= 3; ++x) {
		volume.setVoxel(x, 2, 2, solid);
	}
	// Sculpt only the middle voxel region
	voxel::Region sculptRegion(2, 2, 2, 2, 2, 2);
	const int before = countSolid(volume);
	// Strength 0.5 -> threshold 2. Middle voxel has 2 anchor neighbors -> NOT removed
	sculptErode(volume, sculptRegion, 0.5f, 1);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testSetLevelErode) {
	// Test the set-level API directly
	voxel::Region region(0, 0, 0, 2, 1, 2);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::BitVolume anchors(region);
	const voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 3x3x1 flat plane with a single protrusion
	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			solid.setVoxel(x, 0, z, true);
			voxelMap.setVoxel(x, 0, z, v);
		}
	}
	// Protrusion on top of center
	const glm::ivec3 protrusion(1, 1, 1);
	solid.setVoxel(protrusion, true);
	voxelMap.setVoxel(protrusion, v);

	sculptErode(solid, voxelMap, anchors, 0.5f, 1);
	// Protrusion has 1 face-neighbor (below) -> removed at threshold 2
	EXPECT_FALSE(solid.hasValue(protrusion.x, protrusion.y, protrusion.z));
	EXPECT_FALSE(voxelMap.hasVoxel(protrusion));
}

TEST_F(VolumeSculptTest, testSmoothAdditiveFillsGap) {
	// Two columns side by side: one is 3 tall, the other is 1 tall.
	// SmoothAdditive with PositiveY "up" should fill the short column.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Tall column at (2,0..2,2) - height 3
	volume.setVoxel(2, 0, 2, solid);
	volume.setVoxel(2, 1, 2, solid);
	volume.setVoxel(2, 2, 2, solid);
	// Short column at (3,0,2) - height 1
	volume.setVoxel(3, 0, 2, solid);

	const int before = countSolid(volume);
	// heightThreshold=1: any neighbor 1+ voxel taller triggers fill
	sculptSmoothAdditive(volume, region, voxel::FaceNames::PositiveY, 1, 1, fill);
	EXPECT_GT(countSolid(volume), before);
	// (3,1,2) should be filled - short column grew because neighbor is 2 taller
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 1, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothAdditiveMultipleIterations) {
	// Short column next to tall column, multiple iterations should fill progressively
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Tall column at (2,0..3,2) - height 4
	for (int y = 0; y <= 3; ++y) {
		volume.setVoxel(2, y, 2, solid);
	}
	// Short column at (3,0,2) - height 1
	volume.setVoxel(3, 0, 2, solid);

	// heightThreshold=1, 3 iterations: should fill 1 voxel per column per iteration
	sculptSmoothAdditive(volume, region, voxel::FaceNames::PositiveY, 1, 3, fill);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 1, 2).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 2, 2).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 3, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothAdditiveNoFillWhenSameHeight) {
	// Two columns of equal height - nothing should be filled
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	volume.setVoxel(2, 0, 2, solid);
	volume.setVoxel(2, 1, 2, solid);
	volume.setVoxel(3, 0, 2, solid);
	volume.setVoxel(3, 1, 2, solid);

	const int before = countSolid(volume);
	sculptSmoothAdditive(volume, region, voxel::FaceNames::PositiveY, 1, 1, fill);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testSmoothAdditiveHighThresholdNoFill) {
	// With heightThreshold=10, a height diff of 4 is not enough to trigger fill
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	for (int y = 0; y <= 4; ++y) {
		volume.setVoxel(2, y, 2, solid);
	}
	volume.setVoxel(3, 0, 2, solid);

	const int before = countSolid(volume);
	// heightThreshold=10, height diff is only 4 < 10
	sculptSmoothAdditive(volume, region, voxel::FaceNames::PositiveY, 10, 1, fill);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testSmoothAdditiveOneVoxelPerIteration) {
	// Verify that only one voxel per column is added per iteration
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Tall column height 4, short column height 1
	for (int y = 0; y <= 3; ++y) {
		volume.setVoxel(2, y, 2, solid);
	}
	volume.setVoxel(3, 0, 2, solid);

	const int before = countSolid(volume);
	// 1 iteration, threshold=1: should add exactly 1 voxel at (3,1,2)
	sculptSmoothAdditive(volume, region, voxel::FaceNames::PositiveY, 1, 1, fill);
	EXPECT_EQ(countSolid(volume), before + 1);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 1, 2).getMaterial()));
	// (3,2,2) should still be air after only 1 iteration
	EXPECT_TRUE(voxel::isAir(volume.voxel(3, 2, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodeRemovesEdge) {
	// 3x3x2 block with a single tower voxel on the corner at y=2.
	// The tower voxel is the only top-of-column with <4 planar neighbors at that height.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 3x3x2 base block
	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			volume.setVoxel(x, 0, z, solid);
			volume.setVoxel(x, 1, z, solid);
		}
	}
	// Single tower voxel on corner
	volume.setVoxel(0, 2, 0, solid);

	const int before = countSolid(volume);
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 1);
	// Top layer (y=1) edge/corner voxels removed, plus tower at (0,2,0)
	EXPECT_LT(countSolid(volume), before);
	// The tower voxel should definitely be removed (0 planar neighbors at y=2)
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 2, 0).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodeKeepsSurrounded) {
	// A 3x3x2 block - top center voxel at (1,1,1) has 4 planar neighbors and should NOT be removed
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			volume.setVoxel(x, 0, z, solid);
			volume.setVoxel(x, 1, z, solid);
		}
	}

	const int before = countSolid(volume);
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 1);
	// Center top (1,1,1) has 4 planar neighbors -> stays. Only edge/corner tops removed.
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 1, 1).getMaterial()));
	EXPECT_LT(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testSmoothErodeOnePerIteration) {
	// A 1x3 tower. Top voxel is edge (0 planar neighbors). After 1 iteration, only top removed.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	volume.setVoxel(2, 0, 2, solid);
	volume.setVoxel(2, 1, 2, solid);
	volume.setVoxel(2, 2, 2, solid);

	const int before = countSolid(volume);
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 1);
	// Only top voxel (2,2,2) removed
	EXPECT_EQ(countSolid(volume), before - 1);
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 2, 2).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 1, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodeMultipleIterations) {
	// A 1x3 tower. 2 iterations should remove top 2 voxels.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	volume.setVoxel(2, 0, 2, solid);
	volume.setVoxel(2, 1, 2, solid);
	volume.setVoxel(2, 2, 2, solid);

	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 2);
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 2, 2).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 1, 2).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 0, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodePreserveTopHeight3x3Pyramid) {
	// Base layer at y=0 (5x5) plus a 3x3 tower (y=0..2). Two height groups:
	// base at height 0, tower at height 2. PreserveTopHeight trims the tower
	// island edges while keeping the center at full height.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Wide base at y=0
	for (int x = 0; x <= 4; ++x) {
		for (int z = 0; z <= 4; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// 3x3 tower (y=1..2) on top of base
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 1, z, solid);
			volume.setVoxel(x, 2, z, solid);
		}
	}

	const int before = countSolid(volume);
	// iterations=2, trimPerStep=1: edge columns at dist=1 get trimmed by min(1*1, 2) = 1
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 2, true, 1);
	EXPECT_LT(countSolid(volume), before);
	// Center column (2,2) should keep full height (y=2 still solid)
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 2).getMaterial()));
	// Edge column (1,1) at dist=1 should lose top layer (y=2 removed)
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 2, 1).getMaterial()));
	// But edge column still has y=1
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 1, 1).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodePreserveTopHeightBottomIslandProtected) {
	// Two height groups: a base layer (y=0) and a tower (y=0..2).
	// The base layer is the bottom island and should never be eroded.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Wide base at y=0
	for (int x = 0; x <= 4; ++x) {
		for (int z = 0; z <= 4; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// 3x3 tower on top (y=1..2)
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 1, z, solid);
			volume.setVoxel(x, 2, z, solid);
		}
	}

	// Count base voxels
	int baseCount = 0;
	for (int x = 0; x <= 4; ++x) {
		for (int z = 0; z <= 4; ++z) {
			if (voxel::isBlocked(volume.voxel(x, 0, z).getMaterial())) {
				baseCount++;
			}
		}
	}

	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 3, true, 1);

	// Base layer should be untouched
	int baseCountAfter = 0;
	for (int x = 0; x <= 4; ++x) {
		for (int z = 0; z <= 4; ++z) {
			if (voxel::isBlocked(volume.voxel(x, 0, z).getMaterial())) {
				baseCountAfter++;
			}
		}
	}
	EXPECT_EQ(baseCount, baseCountAfter);
}

TEST_F(VolumeSculptTest, testSmoothErodePreserveTopHeightTrimPerStep) {
	// Base layer at y=0 (7x7) plus a 5x5 tower (y=0..4). trimPerStep=2 trims twice as aggressively.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Wide base at y=0
	for (int x = 0; x <= 6; ++x) {
		for (int z = 0; z <= 6; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// 5x5 tower (y=1..4) on top of base
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			for (int y = 1; y <= 4; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	// With trimPerStep=2, iterations=4: dist=1 columns trim min(1*2, 4) = 2, dist=2 trim min(2*2, 4) = 4
	// But maxTrim caps at (topCoord - bottomHeight) * step - 1 = (4 - 0) - 1 = 3
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 4, true, 2);
	// Center (3,3) at dist=0 keeps full height
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 4, 3).getMaterial()));
	// Dist=1 column (2,3) loses 2 layers: y=4 and y=3 removed
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 4, 3).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 3, 3).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 3).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodePreserveTopHeightNoSinkBelowBottom) {
	// Columns should never sink below bottomHeight + 1
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Base at y=0
	for (int x = 0; x <= 4; ++x) {
		for (int z = 0; z <= 4; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// Short tower at y=1..2 (only 2 layers above base)
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 1, z, solid);
			volume.setVoxel(x, 2, z, solid);
		}
	}

	// High iterations + trimPerStep to try to over-trim
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 10, true, 5);

	// Edge tower columns should still have at least y=1 (bottomHeight=0, so can't sink to 0)
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 1, z).getMaterial()))
				<< "Column (" << x << "," << z << ") sank below bottom+1";
		}
	}
}

} // namespace voxelutil
