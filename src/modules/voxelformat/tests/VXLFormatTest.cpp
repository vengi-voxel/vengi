/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXLFormat.h"

namespace voxel {

class VXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXLFormatTest, testLoad) {
	const io::FilePtr& file = _testApp->filesystem()->open("cc.vxl");
	ASSERT_TRUE((bool)file) << "Could not open vxl file";
	VXLFormat f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load vxl file";
	delete volume;
}

}
