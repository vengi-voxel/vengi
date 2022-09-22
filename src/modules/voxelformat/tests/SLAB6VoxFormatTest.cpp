/**
 * @file
 */

#include "voxelformat/SLAB6VoxFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class SLAB6VoxFormatTest : public AbstractVoxFormatTest {};

TEST_F(SLAB6VoxFormatTest, testSaveSmallVoxel) {
	SLAB6VoxFormat f;
	testSaveLoadVoxel("loadvoxel.vox", &f);
}

} // namespace voxelformat
