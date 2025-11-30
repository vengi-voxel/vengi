/**
 * @file
 */

#include "voxelformat/private/goxel/GoxFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class GoxFormatTest : public AbstractFormatTest {};

TEST_F(GoxFormatTest, testLoad) {
	testLoad("test.gox");
}

TEST_F(GoxFormatTest, testSaveSmallVoxel) {
	GoxFormat f;
	testSaveLoadVoxel("goxel-smallvolumesavetest.gox", &f, -16, 15, voxel::ValidateFlags::None);
}

TEST_F(GoxFormatTest, testLoadRGB) {
	testRGB("rgb.gox");
}

TEST_F(GoxFormatTest, testLoadScreenshot) {
	testLoadScreenshot("chr_knight.gox", 128, 128, color::RGBA(158, 59, 59), 65, 27);
}

} // namespace voxelformat
