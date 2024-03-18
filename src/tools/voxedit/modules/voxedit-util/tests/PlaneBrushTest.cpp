/**
 * @file
 */

#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "palette/Palette.h"
#include "voxel/Voxel.h"

namespace voxedit {

class PlaneBrushTest : public app::AbstractTest {
protected:
};

TEST_F(PlaneBrushTest, testExtrude) {
	PlaneBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(0, 3));
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			volume.setVoxel(x, y, 0, voxel);
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());

	brushContext.cursorFace = voxel::FaceNames::PositiveZ;
	brushContext.cursorVoxel = voxel;
	brushContext.hitCursorVoxel = voxel;

	const int maxZ = 3;
	for (int z = 1; z <= maxZ; ++z) {
		brushContext.cursorPosition = glm::ivec3(1, 1, z);
		EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial())) << "for z: " << z;
		EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext)) << "for z: " << z;
		EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial())) << "for z: " << z;
	}

	brush.shutdown();
}

} // namespace voxedit
