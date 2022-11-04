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

TEST_F(GoxFormatTest, testLoadScreenshot) {
	testLoadScreenshot("chr_knight.gox", 128, 128, core::RGBA(148, 58, 58), 63, 8);
}

}
