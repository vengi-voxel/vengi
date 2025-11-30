/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class MDLFormatTest : public AbstractFormatTest {};

TEST_F(MDLFormatTest, testVoxelize) {
	// model from https://www.moddb.com/groups/share-and-mod/downloads/quake-1-mdl-droid
	// or from https://github.com/QW-Group/ezquake-media/blob/master/game/progs/flame0.mdl
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "flame0.mdl", 1);
	if (IsSkipped()) {
		return;
	}
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(node->name(), "flame1");
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(v, nullptr);
	EXPECT_EQ(v->region().getDimensionsInVoxels(), glm::ivec3(8, 18, 8));
	EXPECT_EQ(178, voxelutil::countVoxels(*v));
	EXPECT_GT(node->palette().colorCount(), 50);
	EXPECT_LT(node->palette().colorCount(), 250);
	// EXPECT_EQ(color::RGBA(0x25, 0x1b, 0x0d), node->palette().color(0));
}

} // namespace voxelformat
