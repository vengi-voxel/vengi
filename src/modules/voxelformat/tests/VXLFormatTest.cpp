/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXLFormat.h"

namespace voxelformat {

class VXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXLFormatTest, testLoad) {
	canLoad("cc.vxl");
}

TEST_F(VXLFormatTest, testLoadRGB) {
	testRGB("rgb.vxl");
}

TEST_F(VXLFormatTest, testSave) {
	VXLFormat f;
	testLoadSaveAndLoad("cc.vxl", f, "cc-save.vxl", f);
}

TEST_F(VXLFormatTest, testSaveSmallVoxel) {
	VXLFormat f;
	testSaveLoadVoxel("cc-smallvolumesavetest.vxl", &f);
}

}
