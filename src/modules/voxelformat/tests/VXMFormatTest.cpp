/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"

namespace voxel {

class VXMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXMFormatTest, DISABLED_testLoad) {
	VXMFormat f;
	std::unique_ptr<RawVolume> volume(load("test.vmx", f));
	ASSERT_NE(nullptr, volume) << "Could not load vmx file";
}

}
