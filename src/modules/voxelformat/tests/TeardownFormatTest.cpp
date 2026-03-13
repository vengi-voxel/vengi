/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

class TeardownFormatTest : public AbstractFormatTest {};

TEST_F(TeardownFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "about.bin", 225);
}

} // namespace voxelformat
