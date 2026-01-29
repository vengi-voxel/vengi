/**
 * @file
 */

#include "voxel/BitVolume.h"
#include "app/tests/AbstractTest.h"
#include "voxel/Voxel.h"

namespace voxel {

class BitVolumeTest : public app::AbstractTest {};

TEST_F(BitVolumeTest, testBasic) {
	const voxel::Region region{glm::ivec3(0), glm::ivec3(63)};
	BitVolume v(region);
	v.setVoxel(1, 2, 1, voxel::createVoxel(VoxelType::Generic, 0));

	EXPECT_EQ(region.voxels() / CHAR_BIT, (int)v.bytes());
	EXPECT_TRUE(v.hasValue(1, 2, 1));
	EXPECT_FALSE(v.hasValue(0, 0, 0));
	EXPECT_FALSE(v.hasValue(-1, -1, -1));
}

TEST_F(BitVolumeTest, testFillAndClear) {
	const voxel::Region region{glm::ivec3(0), glm::ivec3(3)};
	BitVolume v(region);

	// Initially all should be false
	EXPECT_FALSE(v.hasValue(0, 0, 0));
	EXPECT_FALSE(v.hasValue(3, 3, 3));

	// Fill all
	v.fill();
	EXPECT_TRUE(v.hasValue(0, 0, 0));
	EXPECT_TRUE(v.hasValue(3, 3, 3));

	// Clear all
	v.clear();
	EXPECT_FALSE(v.hasValue(0, 0, 0));
	EXPECT_FALSE(v.hasValue(3, 3, 3));
}

TEST_F(BitVolumeTest, testInvert) {
	const voxel::Region region{glm::ivec3(0), glm::ivec3(3)};
	BitVolume v(region);

	// Set one voxel
	v.setVoxel(1, 1, 1, true);
	EXPECT_TRUE(v.hasValue(1, 1, 1));
	EXPECT_FALSE(v.hasValue(0, 0, 0));

	// Invert
	v.invert();
	EXPECT_FALSE(v.hasValue(1, 1, 1));
	EXPECT_TRUE(v.hasValue(0, 0, 0));
}

TEST_F(BitVolumeTest, testResize) {
	const voxel::Region region1{glm::ivec3(0), glm::ivec3(3)};
	BitVolume v(region1);

	v.setVoxel(1, 1, 1, true);
	EXPECT_TRUE(v.hasValue(1, 1, 1));

	// Resize to a larger region
	const voxel::Region region2{glm::ivec3(0), glm::ivec3(7)};
	v.resize(region2);

	// After resize, all bits should be cleared
	EXPECT_FALSE(v.hasValue(1, 1, 1));
	EXPECT_EQ(v.region(), region2);
}

TEST_F(BitVolumeTest, testCopyConstructor) {
	const voxel::Region region{glm::ivec3(0), glm::ivec3(3)};
	BitVolume v1(region);
	v1.setVoxel(1, 1, 1, true);

	BitVolume v2(v1);
	EXPECT_TRUE(v2.hasValue(1, 1, 1));
	EXPECT_EQ(v2.region(), region);

	// Modifying v2 should not affect v1
	v2.setVoxel(2, 2, 2, true);
	EXPECT_FALSE(v1.hasValue(2, 2, 2));
	EXPECT_TRUE(v2.hasValue(2, 2, 2));
}

TEST_F(BitVolumeTest, testDefaultConstruct) {
	BitVolume v;
	EXPECT_FALSE(v.isValid());
}

} // namespace voxel
