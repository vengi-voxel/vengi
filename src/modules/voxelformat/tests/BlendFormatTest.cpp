/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class BlendFormatTest : public AbstractFormatTest {};

TEST_F(BlendFormatTest, DISABLED_testLoad) {
	testLoad("blender-tests-data-cubes-hierarchy.blend", 22);
}

} // namespace voxelformat
