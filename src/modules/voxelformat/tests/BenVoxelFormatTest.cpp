/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class BenVoxelFormatTest : public AbstractFormatTest {};

TEST_F(BenVoxelFormatTest, testLoadJSON) {
	testLoad("sora.ben.json");
}

TEST_F(BenVoxelFormatTest, testLoadBinary) {
	testLoad("sora.ben");
}

} // namespace voxelformat
