/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"

namespace voxelformat {

class VXMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXMFormatTest, DISABLED_testLoadRGB) {
	testRGB("rgb.vxm");
}

TEST_F(VXMFormatTest, testLoad) {
	canLoad("test.vxm");
}

TEST_F(VXMFormatTest, testLoadFileCreatedBySandboxVoxeditVersion12) {
	canLoad("test2.vxm");
}

TEST_F(VXMFormatTest, testSaveVerySmallVoxel) {
	VXMFormat f;
	testSaveSmallVolume("verysmallvolumesavetest.vxm", &f);
}

TEST_F(VXMFormatTest, testSaveSmallVoxel) {
	VXMFormat f;
	testSaveLoadVoxel("sandbox-smallvolumesavetest.vxm", &f);
}

}
