/**
 * @file
 */

#include "voxelformat/private/voxelmax/VMaxFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class VMaxFormatTest : public AbstractVoxFormatTest {};

TEST_F(VMaxFormatTest, DISABLED_testLoad0) {
	canLoad("0voxel.vmax.zip");
}

TEST_F(VMaxFormatTest, testLoad1) {
	canLoad("1voxel.vmax.zip");
}

TEST_F(VMaxFormatTest, testLoad2) {
	canLoad("2voxel.vmax.zip");
}

TEST_F(VMaxFormatTest, testLoad5) {
	canLoad("5voxel.vmax.zip");
}

} // namespace voxelformat
