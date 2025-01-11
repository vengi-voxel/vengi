/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"

namespace voxelformat {

class SpriteStackFormatTest : public AbstractFormatTest {};

TEST_F(SpriteStackFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "spritestack.zip", 1u);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(v, nullptr);
	const palette::Palette &palette = node->palette();
	ASSERT_EQ(palette.colorCount(), 2u) << palette;
	EXPECT_EQ(core::RGBA(0, 0, 0, 0), palette.color(0));
	EXPECT_EQ(core::RGBA(0xa4, 0x2d, 0x27, 0xff), palette.color(1));
}

} // namespace voxelformat
