/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"

namespace voxelformat {

class VXMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXMFormatTest, DISABLED_testRGB) {
	testRGB("rgb.vxm");
}

TEST_F(VXMFormatTest, testLoadVersion4) {
	canLoad("test.vxm");
}

TEST_F(VXMFormatTest, testLoadVersion12) {
	canLoad("test2.vxm");
}

TEST_F(VXMFormatTest, testSaveSmallVolume) {
	VXMFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vxm", &f);
}

TEST_F(VXMFormatTest, testSaveLoadVoxel) {
	VXMFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vxm", &f);
}

}
