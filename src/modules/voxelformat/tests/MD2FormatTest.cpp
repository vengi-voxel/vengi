/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class MD2FormatTest : public AbstractFormatTest {};

TEST_F(MD2FormatTest, testVoxelize) {
	testLoad("cube.md2");
}

} // namespace voxelformat
