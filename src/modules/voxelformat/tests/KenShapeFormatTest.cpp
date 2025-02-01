/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class KenShapeFormatTest : public AbstractFormatTest {};

TEST_F(KenShapeFormatTest, testLoad) {
	testLoad("test.kenshape");
}

} // namespace voxelformat
