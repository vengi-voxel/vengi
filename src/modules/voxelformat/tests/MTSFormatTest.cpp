/**
 * @file
 */

#include "voxelformat/private/minecraft/MTSFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class MTSFormatTest : public AbstractFormatTest {};

TEST_F(MTSFormatTest, DISABLED_testSaveCubeModel) {
	MTSFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	testSaveLoadCube("mts-savecubemodel.mts", &f, flags);
}

TEST_F(MTSFormatTest, DISABLED_testSaveSmallVoxel) {
	MTSFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	testSaveLoadVoxel("mts-smallvolumesavetest.mts", &f, -16, 15, flags);
}

TEST_F(MTSFormatTest, DISABLED_testLoadSave) {
	MTSFormat f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	testConvert("minetest.mts", f, "mts-voxlap5.mts", f, flags);
}

} // namespace voxelformat
