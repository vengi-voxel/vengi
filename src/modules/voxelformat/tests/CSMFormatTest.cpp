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

class CSMFormatTest : public AbstractFormatTest {};

TEST_F(CSMFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "chronovox-studio.csm", 11);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_EQ("Head", node->name());
	const voxel::Region &region = sceneGraph.resolveRegion(*node);
	EXPECT_EQ(0, region.getLowerX());
	EXPECT_EQ(0, region.getLowerY());
	EXPECT_EQ(0, region.getLowerZ());
	EXPECT_EQ(16, region.getUpperX());
	EXPECT_EQ(12, region.getUpperY());
	EXPECT_EQ(11, region.getUpperZ());
	const voxel::RawVolume *v = sceneGraph.resolveVolume(*node);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(1606, voxelutil::countVoxels(*v));
	EXPECT_EQ(79u, v->voxel(7, 2, 11).getColor());
	EXPECT_EQ(191u, v->voxel(6, 4, 10).getColor());
}

} // namespace voxelformat
