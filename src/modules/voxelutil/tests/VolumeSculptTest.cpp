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
	// Both y=3 and y=2 layers should be removed (pushed down twice)
	for (int z = 1; z <= 3; ++z) {
		for (int x = 1; x <= 3; ++x) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 3, z).getMaterial()));
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 2, z).getMaterial()));
		}
	}
	// y=1 should remain
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 1, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testFlattenHollowPreservesCap) {
	// Hollow box: only shell voxels, empty interior
	voxel::Region region(0, 6);
	voxel::RawVolume volume(region);
	const voxel::Voxel shell = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	// Fill entire cube then hollow it out
	fillRegion(volume, voxel::Region(1, 5), shell);
	for (int z = 2; z <= 4; ++z) {
		for (int y = 2; y <= 4; ++y) {
			for (int x = 2; x <= 4; ++x) {
				volume.setVoxel(x, y, z, voxel::Voxel());
			}
		}
	}

	sculptFlatten(volume, region, voxel::FaceNames::PositiveY, 1);
	// Top layer (y=5) removed, but pushed down to y=4 to preserve cap
	for (int z = 1; z <= 5; ++z) {
		for (int x = 1; x <= 5; ++x) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 5, z).getMaterial()));
		}
	}
	// y=4 should now be solid across the full face (cap preserved)
	for (int z = 1; z <= 5; ++z) {
		for (int x = 1; x <= 5; ++x) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 4, z).getMaterial()))
				<< "Expected cap at (" << x << ", 4, " << z << ")";
		}
	}
}

TEST_F(VolumeSculptTest, testFlattenPreservesTopColor) {
	// Top layer has color 5, layer below has color 1. After flatten, layer below gets color 5.
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel bottom = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel top = voxel::createVoxel(voxel::VoxelType::Generic, 5);
	fillRegion(volume, voxel::Region(1, 1, 1, 3, 2, 3), bottom);
	fillRegion(volume, voxel::Region(1, 3, 1, 3, 3, 3), top);

	sculptFlatten(volume, region, voxel::FaceNames::PositiveY, 1);
	// y=3 removed
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 3, 2).getMaterial()));
	// y=2 should now have the top color (5) since the top voxels were pushed down
	EXPECT_EQ(volume.voxel(2, 2, 2).getColor(), 5);
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

TEST_F(VolumeSculptTest, testSmoothErodeRemovesTower) {
	// 3x3x2 block with a single tower voxel on the corner at y=2.
	// The tower column is taller than its neighbors and should be trimmed.
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
	// Tower column (0,0) has top=2, neighbors have top=1. Average=1. Trim to 1.
	EXPECT_LT(countSolid(volume), before);
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 2, 0).getMaterial()));
	// Base slab is flat (all top=1) - should remain untouched
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 1, 1).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothErodeUniformBlockUnchanged) {
	// A 3x3x5 uniform block. All populated neighbors have the same height,
	// so the average equals the top and no column erodes.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			for (int y = 0; y < 5; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	const int before = countSolid(volume);
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 2);
	// Uniform block: all neighbor tops equal, average == top, no erosion
	EXPECT_EQ(countSolid(volume), before) << "Uniform block should not erode";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 4, 0).getMaterial()))
		<< "Corner top should survive";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 4, 1).getMaterial()))
		<< "Center top should survive";
}

TEST_F(VolumeSculptTest, testSmoothErodeIsolatedTowerUnchanged) {
	// A single-column 1x3 tower with no planar neighbors. Only self in kernel,
	// so average equals top and no erosion occurs.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	volume.setVoxel(2, 0, 2, solid);
	volume.setVoxel(2, 1, 2, solid);
	volume.setVoxel(2, 2, 2, solid);

	const int before = countSolid(volume);
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 10);
	// Isolated tower: only self in kernel, average == top, no erosion
	EXPECT_EQ(countSolid(volume), before) << "Isolated tower should not erode";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 2).getMaterial()))
		<< "Top should survive";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 0, 2).getMaterial()))
		<< "Bottom should survive";
}

TEST_F(VolumeSculptTest, testSmoothErodeTowerOnSlab) {
	// A 3x3x1 slab with a 3-tall tower on top at (1,1). The tower should be trimmed
	// toward the slab height, converging over iterations.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 3x3 slab at y=0
	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// Tower at (1,0,1) going up to y=3
	volume.setVoxel(1, 1, 1, solid);
	volume.setVoxel(1, 2, 1, solid);
	volume.setVoxel(1, 3, 1, solid);

	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 3);
	// Tower should be trimmed toward neighbor height (y=0). Bottom voxel at y=0 always survives.
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 1).getMaterial()))
		<< "Base slab voxel must survive";
	// Top of tower should have been trimmed
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 3, 1).getMaterial()))
		<< "Top of tower should be eroded";
	// Slab neighbors should be untouched (all same height)
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()))
		<< "Slab corner should survive";
}

TEST_F(VolumeSculptTest, testSmoothErodeHollowBoxWallsErode) {
	// A hollow box on a wider table. Outer wall columns (top=5) neighbor short
	// table columns (top=0), so their average is pulled down and they erode.
	// Interior roof columns have all same-height neighbors and stay.
	voxel::Region region(0, 9);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build hollow box from (1,1,1) to (5,5,5) - shell only
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			for (int y = 1; y <= 5; ++y) {
				if (x == 1 || x == 5 || z == 1 || z == 5 || y == 1 || y == 5) {
					volume.setVoxel(x, y, z, solid);
				}
			}
		}
	}
	// Table floor at y=0
	for (int x = 0; x <= 6; ++x) {
		for (int z = 0; z <= 6; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}

	// Interior should be air before erode
	EXPECT_TRUE(voxel::isAir(volume.voxel(3, 3, 3).getMaterial()));

	const int before = countSolid(volume);
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 1);

	// Outer wall columns erode (neighbor table columns pull average below 5)
	EXPECT_LT(countSolid(volume), before) << "Outer walls should erode";
	// Interior roof column (3,5,3) has all neighbors at top=5, stays
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 5, 3).getMaterial()))
		<< "Interior roof should survive";
	// Floor should be untouched
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 3).getMaterial()))
		<< "Floor should survive";
}

TEST_F(VolumeSculptTest, testSmoothErodeNeverEmptiesColumn) {
	// A 5x5 table with a 3x3 cup (3 voxels tall) on top. Many iterations should
	// flatten the cup toward the table but never remove any column entirely.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Table: 5x5 at y=0
	for (int x = 0; x < 5; ++x) {
		for (int z = 0; z < 5; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// Cup: 3x3 from y=1 to y=3
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			for (int y = 1; y <= 3; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 10);
	// Every column must still have at least one voxel
	for (int x = 0; x < 5; ++x) {
		for (int z = 0; z < 5; ++z) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 0, z).getMaterial()))
				<< "Column (" << x << "," << z << ") bottom should survive";
		}
	}
	// Cup top should be trimmed
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 3, 2).getMaterial()))
		<< "Cup top center should be eroded toward table level";
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

TEST_F(VolumeSculptTest, testSmoothErodePreserveTopHeightColor) {
	// Top layer has color 7, lower layers have color 1. After erosion with preserveTopHeight,
	// each column's new top should have color 7.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel body = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel top = voxel::createVoxel(voxel::VoxelType::Generic, 7);

	// Wide base at y=0
	for (int x = 0; x <= 6; ++x) {
		for (int z = 0; z <= 6; ++z) {
			volume.setVoxel(x, 0, z, body);
		}
	}
	// 5x5 tower (y=1..3 body, y=4 top color)
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			for (int y = 1; y <= 3; ++y) {
				volume.setVoxel(x, y, z, body);
			}
			volume.setVoxel(x, 4, z, top);
		}
	}

	// iterations=2, trimPerStep=1: dist=1 trims 1, dist=2 trims 2
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 2, true, 1);

	// Center (3,3) at dist=0: keeps y=4, should still be top color
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 4, 3).getMaterial()));
	EXPECT_EQ(volume.voxel(3, 4, 3).getColor(), 7);

	// Dist=1 column (2,3): y=4 eroded, new top at y=3 should have top color 7
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 4, 3).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 3, 3).getMaterial()));
	EXPECT_EQ(volume.voxel(2, 3, 3).getColor(), 7)
		<< "New top after trim=1 should have original top color";

	// Dist=2 column (1,3): y=4,y=3 eroded, new top at y=2 should have top color 7
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 4, 3).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 3, 3).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 2, 3).getMaterial()));
	EXPECT_EQ(volume.voxel(1, 2, 3).getColor(), 7)
		<< "New top after trim=2 should have original top color";
}

TEST_F(VolumeSculptTest, testSmoothErodePreserveTopHeightHollow) {
	// Hollow box: eroding top of edge columns must place a solid cap voxel
	// at the new top position even when the column interior is air.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel shell = voxel::createVoxel(voxel::VoxelType::Generic, 5);

	// Base at y=0 (7x7)
	for (int x = 0; x <= 6; ++x) {
		for (int z = 0; z <= 6; ++z) {
			volume.setVoxel(x, 0, z, shell);
		}
	}
	// 5x5 hollow tower y=1..4 (walls + top cap, interior air)
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			for (int y = 1; y <= 4; ++y) {
				const bool isWall = (x == 1 || x == 5 || z == 1 || z == 5 || y == 4);
				if (isWall) {
					volume.setVoxel(x, y, z, shell);
				}
			}
		}
	}

	// iterations=1, trimPerStep=1: dist=1 columns trim 1
	sculptSmoothErode(volume, region, voxel::FaceNames::PositiveY, 1, true, 1);

	// Center (3,3) at dist=0: keeps y=4
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 4, 3).getMaterial()));

	// Dist=1 column (2,2): y=4 eroded, new cap placed at y=3 (was air inside hollow)
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 4, 2).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 3, 2).getMaterial()))
		<< "Cap voxel should be placed at new top even in hollow interior";
	EXPECT_EQ(volume.voxel(2, 3, 2).getColor(), 5);
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

TEST_F(VolumeSculptTest, testSmoothGaussianLevelsColumns) {
	// Two columns side by side: one is 4 tall, the other is 0 tall.
	// Gaussian smoothing should bring the tall one down and fill the short one.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Tall column at (3, 0..3, 3) - height 4
	for (int y = 0; y <= 3; ++y) {
		volume.setVoxel(3, y, 3, solid);
	}
	// Neighbor column at (4, 0, 3) - height 1
	volume.setVoxel(4, 0, 3, solid);

	const int before = countSolid(volume);
	// kernelSize=1, sigma=1.0, 1 iteration
	sculptSmoothGaussian(volume, region, voxel::FaceNames::PositiveY, 1, 1.0f, 1, fill);
	// Heights should redistribute: total count may change but smoothing should occur
	const int after = countSolid(volume);
	EXPECT_NE(before, after);
}

TEST_F(VolumeSculptTest, testSmoothGaussianNoChangeWhenFlat) {
	// A flat 3x3 surface at y=0 should not change under Gaussian smoothing
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}

	const int before = countSolid(volume);
	sculptSmoothGaussian(volume, region, voxel::FaceNames::PositiveY, 1, 1.0f, 3, fill);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testSmoothGaussianMultipleIterations) {
	// Spike on flat surface should be progressively smoothed
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// 5x5 flat base at y=0
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// Spike at center (3, 1..4, 3) - 4 voxels tall above base
	for (int y = 1; y <= 4; ++y) {
		volume.setVoxel(3, y, 3, solid);
	}

	// Multiple iterations should flatten the spike
	sculptSmoothGaussian(volume, region, voxel::FaceNames::PositiveY, 2, 1.5f, 5, fill);
	// The very top of the spike should be gone
	EXPECT_TRUE(voxel::isAir(volume.voxel(3, 4, 3).getMaterial()));
	// Base should still be intact
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 3).getMaterial()));
}

TEST_F(VolumeSculptTest, testSmoothGaussianLargerKernel) {
	// With kernel size 2 (5x5), smoothing should be more aggressive
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// 5x5 flat base at y=0
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	// 3x3 tower at center (2..4, 1..3, 2..4)
	for (int x = 2; x <= 4; ++x) {
		for (int z = 2; z <= 4; ++z) {
			for (int y = 1; y <= 3; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	const int before = countSolid(volume);
	// kernel=2, sigma=2.0, 3 iterations: should significantly smooth the step
	sculptSmoothGaussian(volume, region, voxel::FaceNames::PositiveY, 2, 2.0f, 3, fill);
	// Center base should still exist
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 3).getMaterial()));
	// Smoothing should change the voxel count (may grow or shrink depending on geometry)
	EXPECT_NE(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testBridgeGapFillsBetweenSurfaces) {
	// Two solid blocks separated by a gap. Lines between boundary voxels fill it.
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Bottom block at y=1
	volume.setVoxel(3, 1, 3, solid);
	// Top block at y=5
	volume.setVoxel(3, 5, 3, solid);

	const int before = countSolid(volume);
	sculptBridgeGap(volume, region, fill);
	EXPECT_GT(countSolid(volume), before);
	// Gap at y=2,3,4 should be filled
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 2, 3).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 3, 3).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 4, 3).getMaterial()));
}

TEST_F(VolumeSculptTest, testBridgeGapNoChangeWhenSolid) {
	// A solid column with no gaps should not change
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	for (int y = 1; y <= 3; ++y) {
		volume.setVoxel(2, y, 2, solid);
	}

	const int before = countSolid(volume);
	sculptBridgeGap(volume, region, fill);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testBridgeGapTwoFlatSurfaces) {
	// Two flat surfaces with a gap between them
	voxel::Region region(0, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Bottom surface at y=1
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 1, z, solid);
		}
	}
	// Top surface at y=4
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 4, z, solid);
		}
	}

	sculptBridgeGap(volume, region, fill);
	// Center columns should have y=2 and y=3 filled
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 2).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 3, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testBridgeGapSingleVoxelNoChange) {
	// A single isolated voxel has no gap to bridge
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel fill = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	volume.setVoxel(2, 2, 2, solid);
	const int before = countSolid(volume);
	sculptBridgeGap(volume, region, fill);
	EXPECT_EQ(countSolid(volume), before);
}

// --- SquashToPlane tests ---

TEST_F(VolumeSculptTest, testSquashToPlaneProjectsToClickedLayer) {
	// 3x3x3 cube. Squash to Y=2 plane. Every column has solid voxels, so all should
	// project to Y=2. Other layers become air.
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);

	const int changed = sculptSquashToPlane(volume, region, voxel::FaceNames::PositiveY, 2);
	EXPECT_GT(changed, 0);
	// Y=2 layer should be fully solid (3x3)
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 2, z).getMaterial()));
		}
	}
	// Y=1 and Y=3 layers should be air
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 1, z).getMaterial()));
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 3, z).getMaterial()));
		}
	}
	// Total solid count should be 9 (one layer of 3x3)
	EXPECT_EQ(countSolid(volume), 9);
}

TEST_F(VolumeSculptTest, testSquashToPlanePreservesNearestColor) {
	// Column with different colors at different heights. The voxel nearest to the
	// plane should donate its color.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);

	// Column at (2,*,2): color 10 at Y=1, color 20 at Y=3
	volume.setVoxel(2, 1, 2, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	volume.setVoxel(2, 3, 2, voxel::createVoxel(voxel::VoxelType::Generic, 20));

	// Squash to Y=3 plane - the voxel AT Y=3 (color 20) is nearest (dist=0)
	sculptSquashToPlane(volume, region, voxel::FaceNames::PositiveY, 3);
	EXPECT_EQ(volume.voxel(2, 3, 2).getColor(), 20);
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 1, 2).getMaterial()));
}

TEST_F(VolumeSculptTest, testSquashToPlaneEmptyColumnNoChange) {
	// Columns without any solid voxels should remain empty.
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Only fill a single column at (2,1..3,2)
	for (int y = 1; y <= 3; ++y) {
		volume.setVoxel(2, y, 2, solid);
	}

	sculptSquashToPlane(volume, region, voxel::FaceNames::PositiveY, 2);
	// The filled column should have exactly 1 voxel at Y=2
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 2, 2).getMaterial()));
	EXPECT_EQ(countSolid(volume), 1);
	// Adjacent empty column should still be empty
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 2, 1).getMaterial()));
}

TEST_F(VolumeSculptTest, testSquashToPlaneNegativeX) {
	// Test with NegativeX face - squash along X axis to X=2 plane.
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillRegion(volume, voxel::Region(1, 3), solid);

	sculptSquashToPlane(volume, region, voxel::FaceNames::NegativeX, 2);
	// X=2 plane should have 3x3 solid voxels
	for (int y = 1; y <= 3; ++y) {
		for (int z = 1; z <= 3; ++z) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, y, z).getMaterial()));
		}
	}
	// X=1 and X=3 should be air
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 2, 2).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(3, 2, 2).getMaterial()));
	EXPECT_EQ(countSolid(volume), 9);
}

// --- Reskin tests ---

TEST_F(VolumeSculptTest, testReskinBlendTiling) {
	// 4x4 flat surface at y=0, 2x2 skin with two colors. Blend mode, Repeat tile.
	// Skin should tile across the surface replacing colors.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 4x4 flat surface
	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// 2x2x1 skin: checkerboard pattern
	voxel::Region skinRegion(0, 0, 0, 1, 1, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	skin.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 11));
	skin.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 12));
	skin.setVoxel(1, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 13));

	ReskinConfig config;
	config.mode = ReskinMode::Blend;
	config.tile = ReskinTile::Repeat;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;
	config.skinUpAxis = math::Axis::Z;

	const int changed = sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	EXPECT_GT(changed, 0);
	// Verify tiling: (0,0) and (2,0) should have same color (period 2 in both axes)
	EXPECT_EQ(volume.voxel(0, 0, 0).getColor(), volume.voxel(2, 0, 0).getColor());
	EXPECT_EQ(volume.voxel(1, 0, 0).getColor(), volume.voxel(3, 0, 0).getColor());
	// And the two should be different
	EXPECT_NE(volume.voxel(0, 0, 0).getColor(), volume.voxel(1, 0, 0).getColor());
	// All voxels still solid
	EXPECT_EQ(countSolid(volume), 16);
}

TEST_F(VolumeSculptTest, testReskinBlendWithOffset) {
	// Same 4x4 surface but with offset (1,1). The pattern should shift.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	voxel::Region skinRegion(0, 0, 0, 1, 1, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	skin.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 11));
	skin.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 12));
	skin.setVoxel(1, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 13));

	// First apply without offset
	ReskinConfig config;
	config.mode = ReskinMode::Blend;
	config.tile = ReskinTile::Repeat;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;
	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	const uint8_t colorAtOriginNoOffset = volume.voxel(0, 0, 0).getColor();

	// Reset surface
	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// Apply with offset
	config.offsetU = 1;
	config.offsetV = 1;
	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	const uint8_t colorAtOriginWithOffset = volume.voxel(0, 0, 0).getColor();

	// Offset should shift the pattern
	EXPECT_NE(colorAtOriginNoOffset, colorAtOriginWithOffset);
}

TEST_F(VolumeSculptTest, testReskinNegateCarves) {
	// Negate mode: skin solid voxels carve into the surface
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 4x4 flat surface
	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// 2x2x1 skin: only one voxel solid (at 0,0,0), rest air
	voxel::Region skinRegion(0, 0, 0, 1, 1, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Negate;
	config.tile = ReskinTile::Repeat;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;

	const int before = countSolid(volume);
	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	// Some voxels should be removed (where skin has solid at tiled positions)
	EXPECT_LT(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testReskinReplaceRemovesWhereAir) {
	// Replace mode: skin air voxels should remove surface voxels
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 2; ++x) {
		for (int z = 0; z < 2; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// 2x2x1 skin: only top-left is solid
	voxel::Region skinRegion(0, 0, 0, 1, 1, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	// (1,0,0), (0,1,0), (1,1,0) are air

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Replace;
	config.tile = ReskinTile::Once;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;

	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	// Only 1 voxel should remain (where skin had solid)
	EXPECT_EQ(countSolid(volume), 1);
}

TEST_F(VolumeSculptTest, testReskinFollowSurface) {
	// Stepped surface: y=0 at x=0, y=1 at x=1. With Follow::Voxel, skin should follow the step.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Low step
	volume.setVoxel(0, 0, 0, surface);
	// High step
	volume.setVoxel(1, 0, 0, surface);
	volume.setVoxel(1, 1, 0, surface);

	// 1x1x2 skin: 2 layers deep, both solid with different colors
	voxel::Region skinRegion(0, 0, 0, 0, 0, 1);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 20)); // top layer
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 21)); // deeper layer

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Blend;
	config.tile = ReskinTile::Repeat;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 2;

	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	// Skin base (Z=0, color 21) placed at surface, peak (Z=1, color 20) one layer above.
	// Low column surface at y=0: base goes to y=0
	EXPECT_EQ(volume.voxel(0, 0, 0).getColor(), 21);
	// High column surface at y=1: base goes to y=1
	EXPECT_EQ(volume.voxel(1, 1, 0).getColor(), 21);
	// High column y=0 is below the surface - not touched by skin, keeps original color
	EXPECT_EQ(volume.voxel(1, 0, 0).getColor(), 1);
}

TEST_F(VolumeSculptTest, testReskinZOffsetNegative) {
	// 4x4 surface 2 voxels thick. zOffset=-1 should apply skin one layer below surface.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 2-voxel thick floor
	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
			volume.setVoxel(x, 1, z, surface);
		}
	}

	// 1x1x1 solid skin
	voxel::Region skinRegion(0, 0, 0, 0, 0, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 30));

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Blend;
	config.tile = ReskinTile::Repeat;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;
	config.zOffset = -1;

	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	// Surface is y=1. zOffset=-1 shifts down: skin applies at y=0.
	// y=0 should have skin color (30)
	EXPECT_EQ(volume.voxel(0, 0, 0).getColor(), 30);
	// y=1 should keep original color (not touched by skin)
	EXPECT_EQ(volume.voxel(0, 1, 0).getColor(), 1);
	EXPECT_EQ(countSolid(volume), 32);
}

TEST_F(VolumeSculptTest, testReskinZOffsetPositive) {
	// zOffset=+1 should apply skin one layer above the surface (floating).
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Single layer floor at y=0
	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// 1x1x1 solid skin
	voxel::Region skinRegion(0, 0, 0, 0, 0, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 30));

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Blend;
	config.tile = ReskinTile::Repeat;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;
	config.zOffset = 1;

	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	// Surface is y=0. zOffset=+1 shifts up: skin applies at y=1 (above surface).
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 1, 0).getMaterial()));
	EXPECT_EQ(volume.voxel(0, 1, 0).getColor(), 30);
	// Original surface untouched
	EXPECT_EQ(volume.voxel(0, 0, 0).getColor(), 1);
}

TEST_F(VolumeSculptTest, testReskinInvertSkin) {
	// Invert: skin solid becomes "remove", skin air becomes "keep" (in Negate mode).
	// Combined with invert: skin solid → keep, skin air → remove.
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 2; ++x) {
		for (int z = 0; z < 2; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// 2x2x1 skin: only (0,0,0) is solid
	voxel::Region skinRegion(0, 0, 0, 1, 1, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));

	// Negate + Invert: where skin is solid → treated as air → keep. Where air → treated as solid → remove.
	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Negate;
	config.tile = ReskinTile::Once;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;
	config.invertSkin = true;

	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	// Position (0,0,0) had skin solid → inverted to air → Negate keeps it
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()));
	// Position (1,0,0) had skin air → inverted to solid → Negate removes it
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 0, 0).getMaterial()));
}

TEST_F(VolumeSculptTest, testReskinNoClipboardNoChange) {
	// Empty skin volume should cause no changes
	voxel::Region region(0, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			volume.setVoxel(x, 0, z, surface);
		}
	}

	// Empty skin (1x1x1 all-air volume)
	voxel::Region skinRegion(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume skin(skinRegion);

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	const int before = countSolid(volume);
	sculptReskin(volume, region, skin, voxel::FaceNames::PositiveY, config);
	EXPECT_EQ(countSolid(volume), before);
}

TEST_F(VolumeSculptTest, testReskinStretchMode) {
	// 4x4 surface with 2x2 skin in Stretch mode using BitVolume/SparseVolume API directly.
	// PositiveY face: perp1=Z(U), perp2=X(V). 4x4 grid at Y=0.
	voxel::Region selRegion(0, 0, 0, 3, 0, 3);
	voxel::BitVolume solid(selRegion);
	voxel::SparseVolume voxelMap;
	const voxel::Voxel surface = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 4; ++z) {
			solid.setVoxel(x, 0, z, true);
			voxelMap.setVoxel(glm::ivec3(x, 0, z), surface);
		}
	}

	// 2x2x1 skin: quadrants with different colors
	voxel::Region skinRegion(0, 0, 0, 1, 1, 0);
	voxel::RawVolume skin(skinRegion);
	skin.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	skin.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 11));
	skin.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 12));
	skin.setVoxel(1, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 13));

	ReskinConfig config;
	config.skinUpAxis = math::Axis::Z;
	config.mode = ReskinMode::Blend;
	config.tile = ReskinTile::Stretch;
	config.follow = ReskinFollow::Voxel;
	config.skinDepth = 1;

	sculptReskin(solid, voxelMap, skin, voxel::FaceNames::PositiveY, config);
	// With PositiveY face: U=Z, V=X. Stretch maps V(X) 0..3 to skin V(Y) 0..1.
	// Nearest-neighbor: X=0,1,2 -> skinV=0, X=3 -> skinV=1.
	// Corner (X=0,Z=0) and corner (X=3,Z=3) should have different colors.
	const voxel::Voxel v00 = voxelMap.voxel(glm::ivec3(0, 0, 0));
	const voxel::Voxel v30 = voxelMap.voxel(glm::ivec3(3, 0, 0));
	const voxel::Voxel v03 = voxelMap.voxel(glm::ivec3(0, 0, 3));
	const voxel::Voxel v33 = voxelMap.voxel(glm::ivec3(3, 0, 3));
	// X=0 and X=3 should differ (different V mapping)
	EXPECT_NE(v00.getColor(), v30.getColor());
	// Z=0 and Z=3 should differ (different U mapping)
	EXPECT_NE(v00.getColor(), v03.getColor());
	// Opposite corners should differ
	EXPECT_NE(v00.getColor(), v33.getColor());
}

} // namespace voxelutil
