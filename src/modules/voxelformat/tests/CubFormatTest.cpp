/**
 * @file
 */

#include "voxelformat/private/cubeworld/CubFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class CubFormatTest : public AbstractFormatTest {};

TEST_F(CubFormatTest, testLoad) {
	testLoad("cw.cub");
}

TEST_F(CubFormatTest, testLoadPalette) {
	CubFormat f;
	palette::Palette pal;
	EXPECT_EQ(5, helper_loadPalette("rgb.cub", helper_filesystemarchive(), f, pal));
}

TEST_F(CubFormatTest, testLoadRGB) {
	testRGB("rgb.cub");
}

TEST_F(CubFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.cub");
}

TEST_F(CubFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.cub");
}

TEST_F(CubFormatTest, testSaveSmallVoxel) {
	CubFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("cw-smallvolumesavetest.cub", &f, 0, 1, flags);
}

} // namespace voxelformat
