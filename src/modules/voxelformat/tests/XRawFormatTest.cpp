/**
 * @file
 */

#include "voxelformat/private/magicavoxel/XRawFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class XRawFormatTest : public AbstractVoxFormatTest {};

TEST_F(XRawFormatTest, testSaveSmallVoxel) {
	XRawFormat f;
	testSaveLoadVoxel("mv-testsavesmallvoxel.xraw", &f);
}

} // namespace voxelformat
