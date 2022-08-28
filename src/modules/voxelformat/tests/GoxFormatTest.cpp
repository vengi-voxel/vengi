/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/GoxFormat.h"

namespace voxelformat {

class GoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(GoxFormatTest, testLoad) {
	canLoad("test.gox");
}

TEST_F(GoxFormatTest, testSaveSmallVoxel) {
	GoxFormat f;
	testSaveLoadVoxel("goxel-smallvolumesavetest.gox", &f, -16, 15, voxel::ValidateFlags::None);
}

TEST_F(GoxFormatTest, testLoadRGB) {
	testRGB("rgb.gox");
}

}
