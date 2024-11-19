/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

class FBXFormatTest : public AbstractFormatTest {};

TEST_F(FBXFormatTest, testLoad) {
	testLoad("chr_knight.fbx", 17);
}

// TODO: VOXELFORMAT: we currently don't have fbx write support
TEST_F(FBXFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.fbx");
}

} // namespace voxelformat
