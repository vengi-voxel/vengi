/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXRFormat.h"

namespace voxel {

class VXRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXRFormatTest, testSaveSmallVoxel) {
	VXRFormat f;
	testSaveLoadVoxel("sandbox-smallvolumesavetest.vxr", &f);
}

}
