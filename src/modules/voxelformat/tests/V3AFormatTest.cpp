/**
 * @file
 */

#include "voxelformat/private/voxel3d/V3AFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class V3AFormatTest : public AbstractFormatTest {};

TEST_F(V3AFormatTest, DISABLED_testSaveSmallVoxel) {
	V3AFormat f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("v3a-smallvolumesavetest.v3a", &f, -1, 1, flags);
}

} // namespace voxelformat
