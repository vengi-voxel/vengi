/**
 * @file
 */

#include "voxelformat/private/cubzh/CubzhFormat.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/cubzh/PCubesFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class CubzhFormatTest : public AbstractVoxFormatTest {};

TEST_F(CubzhFormatTest, testLoadPCUBES) {
	canLoad("particubes.pcubes");
}

TEST_F(CubzhFormatTest, testSaveSmallVoxelPCubes) {
	PCubesFormat f;
	testSaveLoadVoxel("pcubes-smallvolumesavetest.pcubes", &f, 0, 1);
}

// not included - copy the file manually to execute the test
TEST_F(CubzhFormatTest, testLoad3ZH) {
	canLoad("cubzh.3zh");
}

TEST_F(CubzhFormatTest, testLoadPalette) {
	PCubesFormat f;
	palette::Palette pal;
	EXPECT_EQ(96, loadPalette("particubes.pcubes", f, pal));
}

TEST_F(CubzhFormatTest, testSaveSmallVoxel3ZH) {
	CubzhFormat f;
	testSaveLoadVoxel("cubzh-smallvolumesavetest.3zh", &f, 0, 1);
}

} // namespace voxelformat
