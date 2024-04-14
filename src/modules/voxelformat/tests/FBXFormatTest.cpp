/**
 * @file
 */

#include "voxelformat/private/mesh/FBXFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class FBXFormatTest : public AbstractVoxFormatTest {};

TEST_F(FBXFormatTest, DISABLED_testSaveLoadVoxel) {
	FBXFormat f;
	testSaveLoadVoxel("bv-smallvolumesavetest.fbx", &f, 0, 10);
}

} // namespace voxelformat
