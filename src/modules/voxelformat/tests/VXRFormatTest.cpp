/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"

namespace voxel {

class VXRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXRFormatTest, testSaveSmallVoxel) {
	VXMFormat vxm;
	testSave("sandbox-smallvolumesavetest0.vxm", &vxm);
	VXRFormat f;
	testSaveLoadVoxel("sandbox-smallvolumesavetest.vxr", &f);
}

}
