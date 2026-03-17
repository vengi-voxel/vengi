/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/private/mesh/PLYFormat.h"

namespace voxelformat {

class PLYFormatTest : public AbstractFormatTest {};

TEST_F(PLYFormatTest, testVoxelizeAscii) {
	testLoad("ascii.ply");
}

TEST_F(PLYFormatTest, testVoxelizeCube) {
	testLoad("cube.ply");
}

TEST_F(PLYFormatTest, testSaveLoadPointCloud) {
	PLYFormat format;
	testSaveLoadPointCloud("pointcloud-saveload.ply", &format);
}

} // namespace voxelformat
