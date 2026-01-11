/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class KenShapeFormatTest : public AbstractFormatTest {};

TEST_F(KenShapeFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test.kenshape");
	EXPECT_EQ(1u, sceneGraph.size());
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(nullptr, volume);
	const voxel::Region &region = volume->region();
	EXPECT_EQ(glm::ivec3(64, 64, 16), region.getDimensionsInVoxels());
	EXPECT_EQ(42, voxelutil::countVoxels(*volume));
	EXPECT_TRUE(voxel::isAir(volume->voxel(29, 29, 0).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume->voxel(28, 29, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume->voxel(27, 29, 0).getMaterial()));
	EXPECT_EQ(node->palette().size(), 16u);
	EXPECT_EQ(node->name(), "Untitled-1");
}

} // namespace voxelformat
