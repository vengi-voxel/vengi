/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"

namespace voxelformat {

class PNGFormatTest : public AbstractFormatTest {};

TEST_F(PNGFormatTest, testLoadPlane) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "fuel_can.png", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getDimensionsInVoxels(), glm::ivec3(128, 128, 1));
}

TEST_F(PNGFormatTest, testLoadVolume) {
	util::ScopedVarChange scped(cfg::VoxformatImageImportType, "2");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-heightmap.png", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getDimensionsInVoxels(), glm::ivec3(8, 8, 3));
}

TEST_F(PNGFormatTest, testLoadHeightmap) {
	util::ScopedVarChange scped(cfg::VoxformatImageImportType, "1");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-heightmap.png", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getDimensionsInVoxels(), glm::ivec3(8, 255, 8));
}

} // namespace voxelformat