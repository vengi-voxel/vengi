/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/CSMFormat.h"

namespace voxel {

class CSMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(CSMFormatTest, testLoad) {
	CSMFormat f;
	std::unique_ptr<RawVolume> volume(load("chronovox-studio.csm", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
}

}
