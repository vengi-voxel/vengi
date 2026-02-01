/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class MapFormatTest : public AbstractFormatTest {};

TEST_F(MapFormatTest, testVoxelize) {
	util::ScopedVarChange scoped(cfg::VoxformatScale, "0.001");
	scenegraph::SceneGraph sceneGraph;
	// this is the workshop map that I created for ufoai
	testLoad(sceneGraph, "test.map", 9);
}

TEST_F(MapFormatTest, testVoxelizeQuads) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-uforadiant.map", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(33, node->region().getDepthInVoxels());
	EXPECT_EQ(2, node->region().getHeightInVoxels());
	EXPECT_EQ(33, node->region().getWidthInVoxels());
	EXPECT_EQ(1024, voxelutil::countVoxels(*node->volume()));
}

TEST_F(MapFormatTest, testVoxelizeSmall) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-uforadiant2.map", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(2, node->region().getHeightInVoxels());
}

TEST_F(MapFormatTest, testVoxelizeTriangle) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-uforadiant-tri.map", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(26, node->region().getDepthInVoxels());
	EXPECT_EQ(2, node->region().getHeightInVoxels());
	EXPECT_EQ(25, node->region().getWidthInVoxels());
}

} // namespace voxelformat
