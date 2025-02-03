/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

class GodotSceneFormatTest : public AbstractFormatTest {};

TEST_F(GodotSceneFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "rgb.qb");
	helper_saveSceneGraph(sceneGraph, "exportrgb.escn");
}

} // namespace voxelformat
