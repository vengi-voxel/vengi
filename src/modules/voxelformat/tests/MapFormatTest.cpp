/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "util/VarUtil.h"

namespace voxelformat {

class MapFormatTest : public AbstractFormatTest {};

TEST_F(MapFormatTest, testVoxelize) {
	util::ScopedVarChange scoped(cfg::VoxformatScale, "0.01");
	scenegraph::SceneGraph sceneGraph;
	// this is the workshop map that I created for ufoai
	testLoad(sceneGraph, "test.map", 10);
}

} // namespace voxelformat
