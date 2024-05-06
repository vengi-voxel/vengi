/**
 * @file
 */

#include "voxelformat/private/sandbox/VXMFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class VXMFormatTest : public AbstractFormatTest {};

TEST_F(VXMFormatTest, testRGB) {
	testRGB("rgb.vxm");
}

TEST_F(VXMFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.vxm");
}

TEST_F(VXMFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.vxm");
}

TEST_F(VXMFormatTest, testLoadVersion4) {
	testLoad("test.vxm");
}

TEST_F(VXMFormatTest, testLoadVersion12) {
	testLoad("test2.vxm");
}

TEST_F(VXMFormatTest, testSaveSmallVolume) {
	VXMFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vxm", &f);
}

TEST_F(VXMFormatTest, testSaveLoadVoxel) {
	VXMFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vxm", &f);
}

} // namespace voxelformat
