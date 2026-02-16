/**
 * @file
 */

#include "voxedit-util/modifier/brush/TextureBrush.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/VoxelUtil.h"
#include "voxel/tests/VoxelPrinter.h"
#include "math/tests/TestMathHelper.h"

namespace voxedit {

class TextureBrushTest : public app::AbstractTest {
protected:
	void prepare(AABBBrush &brush, BrushContext &brushContext, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		brushContext.cursorPosition = mins;
		brushContext.cursorFace = voxel::FaceNames::PositiveX;
		EXPECT_TRUE(brush.beginBrush(brushContext));
		if (brush.singleMode()) {
			EXPECT_FALSE(brush.active());
		} else {
			EXPECT_TRUE(brush.active());
			brushContext.cursorPosition = maxs;
			brush.step(brushContext);
		}
	}
};

TEST_F(TextureBrushTest, testExecuteFilled) {
	TextureBrush brush(nullptr);
	BrushContext brushContext;
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(20, 20, 20);
	prepare(brush, brushContext, {maxs.x, mins.y, mins.z}, {maxs.x, maxs.y, maxs.z});
	ASSERT_TRUE(brush.init());
	brush.setImage(image::loadImage("test-palette-in.png"));
	brush.setUV0(glm::vec2(0.0f, 0.0f));
	brush.setUV1(glm::vec2(1.0f, 1.0f));
	brush.setProjectOntoSurface(true);

	voxel::RawVolume volume(voxel::Region(mins, maxs));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	voxel::RawVolumeWrapper rvWrapper(&volume);
	voxelutil::fill(rvWrapper, brushContext.cursorVoxel);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());

	EXPECT_EQ(wrapper.voxel({20, 10, 10}).getColor(), 1);

	brush.preExecute(brushContext, wrapper.volume());
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	EXPECT_EQ(wrapper.voxel({20, 10, 10}).getColor(), 253);

	brush.shutdown();
}

TEST_F(TextureBrushTest, testCreateImageFromFace) {
	TextureBrush brush(nullptr);
	ASSERT_TRUE(brush.init());

	palette::Palette palette;
	palette.nippon();
	const voxel::Region region(0, 0, 0, 7, 7, 7);
	voxel::RawVolume volume(region);

	// fill the volume with colored voxels
	for (int x = 0; x <= 7; ++x) {
		for (int y = 0; y <= 7; ++y) {
			for (int z = 0; z <= 7; ++z) {
				const uint8_t colorIdx = (uint8_t)((x + y * 8 + z * 64) % 255 + 1);
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, colorIdx));
			}
		}
	}

	EXPECT_TRUE(brush.createImageFromFace(&volume, palette, region, voxel::FaceNames::Front));
	ASSERT_TRUE(brush.image());
	EXPECT_TRUE(brush.image()->isLoaded());
	// Front face: width = x dimension = 8, height = y dimension = 8
	EXPECT_EQ(8, brush.image()->width());
	EXPECT_EQ(8, brush.image()->height());
	// UV should be reset
	EXPECT_EQ(glm::vec2(0.0f), brush.uv0());
	EXPECT_EQ(glm::vec2(1.0f), brush.uv1());

	brush.shutdown();
}

TEST_F(TextureBrushTest, testCreateImageFromFaceInvalidFace) {
	TextureBrush brush(nullptr);
	ASSERT_TRUE(brush.init());

	palette::Palette palette;
	palette.nippon();
	const voxel::Region region(0, 0, 0, 3, 3, 3);
	voxel::RawVolume volume(region);

	EXPECT_FALSE(brush.createImageFromFace(&volume, palette, region, voxel::FaceNames::Max));
	EXPECT_FALSE(brush.createImageFromFace(nullptr, palette, region, voxel::FaceNames::Front));

	brush.shutdown();
}

} // namespace voxedit
