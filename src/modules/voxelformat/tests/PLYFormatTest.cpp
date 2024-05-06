/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class PLYFormatTest : public AbstractFormatTest {};

TEST_F(PLYFormatTest, testVoxelizeAscii) {
	testLoad("ascii.ply");
}

TEST_F(PLYFormatTest, testVoxelizeCube) {
	testLoad("cube.ply");
}

} // namespace voxelformat
