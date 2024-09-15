/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class Autodesk3DSFormatTest : public AbstractFormatTest {};

TEST_F(Autodesk3DSFormatTest, testLoad) {
	testLoad("abrams3.3ds", 3);
}

} // namespace voxelformat
