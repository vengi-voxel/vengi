/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class BenVoxelFormatTest : public AbstractFormatTest {};

TEST_F(BenVoxelFormatTest, DISABLED_testLoad) {
	testLoad("sora.ben.json");
}

} // namespace voxelformat
