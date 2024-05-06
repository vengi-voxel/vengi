/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class CSMFormatTest : public AbstractFormatTest {};

TEST_F(CSMFormatTest, testLoad) {
	testLoad("chronovox-studio.csm", 11);
}

} // namespace voxelformat
