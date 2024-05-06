/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class STLFormatTest : public AbstractFormatTest {};

TEST_F(STLFormatTest, testVoxelizeAscii) {
	testLoad("ascii.stl");
}

TEST_F(STLFormatTest, testVoxelizeCube) {
	testLoad("cube.stl");
}

} // namespace voxelformat
