/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class BenVoxelFormatTest : public AbstractFormatTest {};

TEST_F(BenVoxelFormatTest, DISABLED_testLoadJSON) {
	testLoad("sora.ben.json");
}

TEST_F(BenVoxelFormatTest, DISABLED_testLoadBinary) {
	testLoad("sora.ben");
}

} // namespace voxelformat
