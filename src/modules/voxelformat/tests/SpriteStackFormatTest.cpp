/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class SpriteStackFormatTest : public AbstractFormatTest {};

TEST_F(SpriteStackFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "spritestack.zip", 1u);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(v, nullptr);

	// Validate palette
	const palette::Palette &palette = node->palette();
	ASSERT_EQ(2, palette.colorCount()) << palette;
	EXPECT_EQ(core::RGBA(0, 0, 0, 0), palette.color(0));
	EXPECT_EQ(core::RGBA(0xa4, 0x2d, 0x27, 0xff), palette.color(1));

	// slices.json: 70 slices, 352x244 per slice
	const voxel::Region &region = v->region();
	EXPECT_EQ(352, region.getWidthInVoxels()) << "Width should match slices.json width";
	EXPECT_EQ(244, region.getHeightInVoxels()) << "Height should match slices.json height";
	EXPECT_EQ(70, region.getDepthInVoxels()) << "Depth should match slices.json slices count";

	// The slices.png contains 5611 non-transparent pixels
	const int voxelCount = voxelutil::countVoxels(*v);
	EXPECT_EQ(5611, voxelCount) << "Should have 5611 opaque voxels from non-transparent pixels";
}

} // namespace voxelformat
