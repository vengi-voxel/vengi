/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class LDrawFormatTest : public AbstractFormatTest {};

TEST_F(LDrawFormatTest, testLoadLDR) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "ldraw-simple.ldr", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(node->name(), "ldraw-simple.ldr");
	EXPECT_EQ(node->property(scenegraph::PropAuthor), "Test");
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(volume, nullptr);
	EXPECT_EQ(volume->region().getDimensionsInVoxels(), glm::ivec3(21, 21, 21));
	EXPECT_EQ(voxelutil::countVoxels(*volume), 1920);
}

TEST_F(LDrawFormatTest, testLoadMPD) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "ldraw-simple.mpd", 2);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(node->name(), "main.ldr");
	EXPECT_EQ(node->property(scenegraph::PropAuthor), "Test");
}

} // namespace voxelformat
