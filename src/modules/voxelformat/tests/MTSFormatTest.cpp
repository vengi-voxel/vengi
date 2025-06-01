/**
 * @file
 */

#include "voxelformat/private/minecraft/MTSFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class MTSFormatTest : public AbstractFormatTest {};

TEST_F(MTSFormatTest, testSaveCubeModel) {
	MTSFormat f;
	// this is converted to minecraft block ids and when loaded, we are using the minecraft palette
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
	testSaveLoadCube("mts-savecubemodel.mts", &f, flags);
}

TEST_F(MTSFormatTest, testSaveSmallVoxel) {
	MTSFormat f;
	// this is converted to minecraft block ids and when loaded, we are using the minecraft palette
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
	testSaveLoadVoxel("mts-smallvolumesavetest.mts", &f, 0, 15, flags);
}

TEST_F(MTSFormatTest, DISABLED_testLoadSave) {
	MTSFormat f;
	// this is converted to minecraft block ids and when loaded, we are using the minecraft palette
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
	testConvert("minetest.mts", f, "mts-voxlap5.mts", f, flags);
}

} // namespace voxelformat
