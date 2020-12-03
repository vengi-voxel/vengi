/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/MCRFormat.h"

namespace voxel {

class MCRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(MCRFormatTest, DISABLED_testLoad) {
	MCRFormat f;
	std::unique_ptr<RawVolume> volume(load("minecraft_113.mca", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
}

}
