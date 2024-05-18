/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class QuakeBSPFormatTest : public AbstractFormatTest {};

TEST_F(QuakeBSPFormatTest, testLoad) {
	testLoad("ufoai.bsp");
}

} // namespace voxelformat
