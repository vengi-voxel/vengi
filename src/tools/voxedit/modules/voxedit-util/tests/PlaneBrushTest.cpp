/**
 * @file
 */

#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "AbstractBrushTest.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace voxedit {

class PlaneBrushTest : public app::AbstractTest {
protected:
	void prepare(PlaneBrush &brush, const voxel::Voxel &voxel, BrushContext &brushContext, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		brushContext.cursorVoxel = voxel;
		brushContext.hitCursorVoxel = brushContext.cursorVoxel;
		brushContext.cursorPosition = mins;
		brushContext.cursorFace = voxel::FaceNames::PositiveZ;
		EXPECT_TRUE(brush.start(brushContext));
		ASSERT_FALSE(brush.singleMode());
		EXPECT_TRUE(brush.active());
		brushContext.cursorPosition = maxs;
		brush.step(brushContext);
	}
};

TEST_F(PlaneBrushTest, testExtrude) {
	PlaneBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(0, 3));

	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			volume.setVoxel(x, y, 0, voxel);
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());

	const int maxZ = 3;
	for (int z = 1; z <= maxZ; ++z) {
		prepare(brush, voxel, brushContext, glm::ivec3(1, 1, z), glm::ivec3(1, 1, z));
		EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial())) << "for z: " << z;
		EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext)) << "for z: " << z;
		EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()))
			<< "for z: " << z << " " << wrapper.dirtyRegion().toString();
		brush.stop(brushContext);
	}

	brush.shutdown();
}

TEST_P(BrushTestParamTest, testPlaneBrush) {
	PlaneBrush brush;
	testPlaceAndOverride(brush);
}

} // namespace voxedit
