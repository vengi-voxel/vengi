/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class FBXFormatTest : public AbstractFormatTest {};

TEST_F(FBXFormatTest, testLoad) {
	testLoad("chr_knight.fbx", 17);
}

} // namespace voxelformat
