/**
 * @file
 */

#include "voxedit-util/modifier/brush/PathBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxedit {

class PathBrushTest : public app::AbstractTest {};

TEST_F(PathBrushTest, testExecute) {
	PathBrush brush;
	ASSERT_TRUE(brush.init());
	voxel::Region region(-3, 3);
	voxel::RawVolume volume(region);
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			volume.setVoxel(x, region.getLowerY(), z, voxel::Voxel(voxel::VoxelType::Generic, 0));
		}
	}
	scenegraph::SceneGraph sceneGraph;
	int nodeId;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&volume, false);
		nodeId = sceneGraph.emplace(core::move(node));
	}
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	BrushContext brushContext;
	brushContext.referencePos = volume.region().getLowerCorner();
	brushContext.referencePos.y++; // one above the ground
	brushContext.cursorPosition = volume.region().getUpperCorner();
	brushContext.cursorPosition.y = brushContext.referencePos.y; // one above the ground
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, 0);

	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.referencePos).getMaterial()));

	brush.preExecute(brushContext, wrapper.volume());
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial())) << wrapper.dirtyRegion().toString();
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.referencePos).getMaterial())) << wrapper.dirtyRegion().toString();

	brush.shutdown();
}

} // namespace voxedit
