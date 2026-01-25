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

} // namespace voxel
