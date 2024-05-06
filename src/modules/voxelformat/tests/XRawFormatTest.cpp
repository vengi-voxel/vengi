/**
 * @file
 */

#include "voxelformat/private/magicavoxel/XRawFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class XRawFormatTest : public AbstractFormatTest {};

TEST_F(XRawFormatTest, testSaveSmallVoxel) {
	XRawFormat f;
	testSaveLoadVoxel("mv-testsavesmallvoxel.xraw", &f, 0, 1,
					  voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

} // namespace voxelformat
