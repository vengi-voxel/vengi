/**
 * @file
 */

#include "voxelformat/VBXFormat.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class VBXFormatTest : public AbstractVoxFormatTest {};

TEST_F(VBXFormatTest, testLoad) {
	canLoad("voxelbuilder.vbx");
}

} // namespace voxelformat
