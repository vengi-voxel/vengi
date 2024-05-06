/**
 * @file
 */

#include "voxelformat/private/slab6/SLAB6VoxFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SLAB6VoxFormatTest : public AbstractFormatTest {};

TEST_F(SLAB6VoxFormatTest, testSaveSmallVoxel) {
	SLAB6VoxFormat f;
	testSaveLoadVoxel("loadvoxel.vox", &f, 0, 1, voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

} // namespace voxelformat
