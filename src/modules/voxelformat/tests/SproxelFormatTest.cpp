/**
 * @file
 */

#include "voxelformat/private/sproxel/SproxelFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SproxelFormatTest : public AbstractFormatTest {};

TEST_F(SproxelFormatTest, testLoadRGB) {
	testRGB("rgb.csv");
}

TEST_F(SproxelFormatTest, testSaveSmallVoxel) {
	SproxelFormat f;
	testSaveLoadVoxel("sproxel-smallvolumesavetest.csv", &f, 0, 1, voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

} // namespace voxelformat
