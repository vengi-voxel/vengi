/**
 * @file
 */

#include "voxelformat/private/cubzh/CubzhFormat.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class CubzhFormatTest : public AbstractVoxFormatTest {};

TEST_F(CubzhFormatTest, testLoad) {
	canLoad("particubes.pcubes");
}

TEST_F(CubzhFormatTest, testLoadPalette) {
	CubzhFormat f;
	voxel::Palette pal;
	EXPECT_EQ(96, loadPalette("particubes.pcubes", f, pal));
}

TEST_F(CubzhFormatTest, testSaveSmallVoxel) {
	CubzhFormat f;
	testSaveLoadVoxel("cubzh-smallvolumesavetest.3zh", &f, 0, 1);
}

} // namespace voxelformat
