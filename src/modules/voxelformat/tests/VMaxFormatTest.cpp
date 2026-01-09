/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class VMaxFormatTest : public AbstractFormatTest {};

TEST_F(VMaxFormatTest, testLoad) {
	ASSERT_TRUE(io::isA("0voxel.vmax.zip", voxelformat::voxelLoad()));
}

TEST_F(VMaxFormatTest, DISABLED_testTransform) {
	// this is the same model as test-transform.vox but in vmax format
	scenegraph::SceneGraph sceneGraphVMAX;
	testTransform(sceneGraphVMAX, "test-transform.vmax.zip");

	scenegraph::SceneGraph sceneGraphVOX;
	testLoad(sceneGraphVOX, "test-transform.vox", 20);

	const voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	voxel::sceneGraphComparator(sceneGraphVMAX, sceneGraphVOX, flags);
}

TEST_F(VMaxFormatTest, testLoad0) {
	// Node 'snapshots' is empty - this scene doesn't contain anything
	testLoad("0voxel.vmax.zip", 0);
}

TEST_F(VMaxFormatTest, testLoad1) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "1voxel.vmax.zip", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(voxelutil::countVoxels(*node->volume()), 1);
}

TEST_F(VMaxFormatTest, testLoad2) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "2voxel.vmax.zip");
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(voxelutil::countVoxels(*node->volume()), 2);
}

TEST_F(VMaxFormatTest, testLoad5) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "5voxel.vmax.zip");
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(voxelutil::countVoxels(*node->volume()), 5);
}

TEST_F(VMaxFormatTest, testLoad5Screenshot) {
	color::RGBA color(251, 251, 251, 255);
	testLoadScreenshot("5voxel.vmax.zip", 1280, 1280, color, 1, 1);
}

} // namespace voxelformat
