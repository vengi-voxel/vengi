/**
 * @file
 */

#include "voxedit-util/modifier/brush/PaintBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class PaintBrushTest : public app::AbstractTest {
protected:
	const uint8_t paintColorIndex = 1;
	const uint8_t existingVoxelColorIndex = 0;
	const uint8_t existingNormalIndex = 5;

	// create a volume that has voxels on the ground that we can paint
	int prepareSceneGraph(scenegraph::SceneGraph &sceneGraph) {
		voxel::Region region(-6, 6);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				volume->setVoxel(x, region.getLowerY(), z,
								 voxel::Voxel(voxel::VoxelType::Generic, existingVoxelColorIndex, existingNormalIndex));
			}
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume, true);
		return sceneGraph.emplace(core::move(node));
	}

	void prepareBrushContext(BrushContext &brushContext) {
		brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, paintColorIndex);
	}
};

TEST_F(PaintBrushTest, testExecuteSingle) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSingleMode();

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	ModifierVolumeWrapper wrapper(sceneGraph.node(nodeId), brush.modifierType());

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCorner();

	brush.preExecute(brushContext, wrapper.volume());
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	const voxel::Voxel voxel = wrapper.voxel(brushContext.cursorPosition);
	EXPECT_EQ((int)voxel.getColor(), (int)paintColorIndex) << "Voxel color was not changed by the paint brush";
	EXPECT_EQ((int)voxel.getNormal(), (int)existingNormalIndex) << "Voxel normal was changed by the paint brush";
	const voxel::Voxel voxel1 =
		wrapper.voxel(brushContext.cursorPosition.x + 1, brushContext.cursorPosition.y, brushContext.cursorPosition.z);
	EXPECT_EQ((int)voxel1.getColor(), (int)existingVoxelColorIndex) << "Voxel color was changed by the paint brush";
	EXPECT_EQ((int)voxel1.getNormal(), (int)existingNormalIndex) << "Voxel normal was changed by the paint brush";
	const voxel::Voxel voxel2 = wrapper.voxel(brushContext.cursorPosition.x + 1, brushContext.cursorPosition.y,
											  brushContext.cursorPosition.z + 1);
	EXPECT_EQ((int)voxel2.getColor(), (int)existingVoxelColorIndex) << "Voxel color was changed by the paint brush";
	EXPECT_EQ((int)voxel2.getNormal(), (int)existingNormalIndex) << "Voxel normal was changed by the paint brush";

	brush.shutdown();
}

TEST_F(PaintBrushTest, testExecuteSingleRadius) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSingleMode();
	brush.setRadius(1);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	ModifierVolumeWrapper wrapper(sceneGraph.node(nodeId), brush.modifierType());

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();

	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	const voxel::Voxel voxel = wrapper.voxel(brushContext.cursorPosition);
	EXPECT_EQ((int)voxel.getColor(), (int)paintColorIndex) << "Voxel color was not changed by the paint brush";
	const voxel::Voxel voxel1 =
		wrapper.voxel(brushContext.cursorPosition.x + 1, brushContext.cursorPosition.y, brushContext.cursorPosition.z);
	EXPECT_EQ((int)voxel1.getColor(), (int)paintColorIndex) << "Voxel color was not changed by the paint brush";
	const voxel::Voxel voxel2 = wrapper.voxel(brushContext.cursorPosition.x + 1, brushContext.cursorPosition.y,
											  brushContext.cursorPosition.z + 1);
	EXPECT_EQ((int)voxel2.getColor(), (int)paintColorIndex) << "Voxel color was not changed by the paint brush";

	brush.shutdown();
}

} // namespace voxedit
