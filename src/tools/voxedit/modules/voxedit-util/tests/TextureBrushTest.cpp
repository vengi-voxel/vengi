/**
 * @file
 */

#include "voxedit-util/modifier/brush/TextureBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
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
		EXPECT_TRUE(brush.start(brushContext));
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
	TextureBrush brush;
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
	SelectionManagerPtr selectionMgr = core::make_shared<SelectionManager>();
	ModifierVolumeWrapper wrapper(node, brush.modifierType(), selectionMgr);

	EXPECT_EQ(wrapper.voxel({20, 10, 10}).getColor(), 1);

	brush.preExecute(brushContext, wrapper.volume());
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	brush.postExecute(brushContext);

	EXPECT_EQ(wrapper.voxel({20, 10, 10}).getColor(), 253);

	brush.shutdown();
}

} // namespace voxedit
