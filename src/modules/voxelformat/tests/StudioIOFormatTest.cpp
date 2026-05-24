/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class StudioIOFormatTest : public AbstractFormatTest {};

TEST_F(StudioIOFormatTest, testLoadEncryptedIO) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "studioio-encrypted.io", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(volume, nullptr);
	EXPECT_GT(voxelutil::countVoxels(*volume), 0);
}

} // namespace voxelformat
