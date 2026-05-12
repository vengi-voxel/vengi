/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

class LuantiWorldEditFormatTest : public AbstractFormatTest {};

TEST_F(LuantiWorldEditFormatTest, testLoadLamp) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "luanti-worldedit-lamp.we", 1);
	if (IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(3, node->region().getWidthInVoxels());
	EXPECT_EQ(4, node->region().getHeightInVoxels());
	EXPECT_EQ(3, node->region().getDepthInVoxels());

	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(nullptr, volume);
	EXPECT_TRUE(voxel::isAir(volume->voxel(0, 0, 0).getMaterial()));

	const palette::Palette &palette = node->palette();

	const voxel::Voxel fence = volume->voxel(1, 0, 1);
	EXPECT_FALSE(voxel::isAir(fence.getMaterial()));
	EXPECT_EQ("default:fence_wood", palette.colorName(fence.getColor()));

	const voxel::Voxel lamp = volume->voxel(1, 3, 1);
	EXPECT_FALSE(voxel::isAir(lamp.getMaterial()));
	EXPECT_EQ("wool:grey", palette.colorName(lamp.getColor()));

	const voxel::Voxel torch = volume->voxel(0, 3, 1);
	EXPECT_FALSE(voxel::isAir(torch.getMaterial()));
	EXPECT_EQ("default:torch", palette.colorName(torch.getColor()));
}

} // namespace voxelformat
