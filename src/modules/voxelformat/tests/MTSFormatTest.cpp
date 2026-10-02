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

TEST_F(MTSFormatTest, testLoadSave) {
	MTSFormat f;
	// converted to minecraft block ids; load uses the minecraft palette
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All &
									  ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color |
										voxel::ValidateFlags::Pivot);
	testSaveLoadVoxel("mts-loadsave.mts", &f, 0, 15, flags);
}

} // namespace voxelformat
