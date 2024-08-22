/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class VXBFormatTest : public AbstractFormatTest {};

TEST_F(VXBFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "sandbox-block2.vxb", 1);
}

} // namespace voxelformat
