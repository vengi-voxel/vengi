/**
 * @file
 */

#include "voxedit-util/modifier/brush/NormalBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class NormalBrushTest : public app::AbstractTest {
protected:
	const uint8_t existingColorIndex = 3;
	const uint8_t normalIndex = 7;

	// create a volume that has voxels on the ground
	int prepareSceneGraph(scenegraph::SceneGraph &sceneGraph) {
		voxel::Region region(-6, 6);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				volume->setVoxel(x, region.getLowerY(), z,
								 voxel::Voxel(voxel::VoxelType::Generic, existingColorIndex, NO_NORMAL));
			}
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume, true);
		return sceneGraph.emplace(core::move(node));
	}

	void prepareBrushContext(BrushContext &brushContext) {
		brushContext.normalIndex = normalIndex;
	}
};

TEST_F(NormalBrushTest, testExecuteSingleManual) {
	NormalBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSingleMode();
	brush.setPaintMode(NormalBrush::PaintMode::Manual);

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
	EXPECT_EQ((int)voxel.getNormal(), (int)normalIndex) << "Voxel normal was not changed by the normal brush";
	EXPECT_EQ((int)voxel.getColor(), (int)existingColorIndex) << "Voxel color was changed by the normal brush";

	// verify adjacent voxels were not affected
	const voxel::Voxel voxel1 =
		wrapper.voxel(brushContext.cursorPosition.x + 1, brushContext.cursorPosition.y, brushContext.cursorPosition.z);
	EXPECT_EQ((int)voxel1.getNormal(), (int)NO_NORMAL) << "Voxel normal was changed by the normal brush";
	EXPECT_EQ((int)voxel1.getColor(), (int)existingColorIndex) << "Voxel color was changed by the normal brush";

	const voxel::Voxel voxel2 = wrapper.voxel(brushContext.cursorPosition.x + 1, brushContext.cursorPosition.y,
											  brushContext.cursorPosition.z + 1);
	EXPECT_EQ((int)voxel2.getNormal(), (int)NO_NORMAL) << "Voxel normal was changed by the normal brush";
	EXPECT_EQ((int)voxel2.getColor(), (int)existingColorIndex) << "Voxel color was changed by the normal brush";

	brush.shutdown();
}

} // namespace voxedit
