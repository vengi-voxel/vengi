/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class VBXFormatTest : public AbstractFormatTest {};

TEST_F(VBXFormatTest, testLoad) {
	testLoad("voxelbuilder.vbx");
}

} // namespace voxelformat
