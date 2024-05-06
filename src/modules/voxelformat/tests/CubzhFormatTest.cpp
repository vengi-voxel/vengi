/**
 * @file
 */

#include "voxelformat/private/cubzh/CubzhFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/cubzh/PCubesFormat.h"

namespace voxelformat {

class CubzhFormatTest : public AbstractFormatTest {};

TEST_F(CubzhFormatTest, testLoadPCUBES) {
	testLoad("particubes.pcubes");
}

TEST_F(CubzhFormatTest, testSaveSmallVoxelPCubes) {
	PCubesFormat f;
	testSaveLoadVoxel("pcubes-smallvolumesavetest.pcubes", &f, 0, 1);
}

// not included - copy the file manually to execute the test
TEST_F(CubzhFormatTest, testLoad3ZH) {
	testLoad("cubzh.3zh");
}

TEST_F(CubzhFormatTest, testLoadPalette) {
	PCubesFormat f;
	palette::Palette pal;
	EXPECT_EQ(96, helper_loadPalette("particubes.pcubes", f, pal));
}

TEST_F(CubzhFormatTest, testSaveSmallVoxel3ZH) {
	CubzhFormat f;
	testSaveLoadVoxel("cubzh-smallvolumesavetest.3zh", &f, 0, 1);
}

} // namespace voxelformat
