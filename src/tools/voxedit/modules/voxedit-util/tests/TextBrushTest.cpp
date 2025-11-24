/**
 * @file
 */

#include "voxedit-util/modifier/brush/TextBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"

namespace voxedit {

class TextBrushTest : public app::AbstractTest {};

TEST_F(TextBrushTest, testExecute) {
	TextBrush brush;
	BrushContext brushContext;
	brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	ASSERT_TRUE(brush.init());
	brush.setInput("ABC");
	brush.setFont("font.ttf");
	voxel::RawVolume volume(voxel::Region(0, 0, 0, 20, 20, 4));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	scenegraph::SceneGraph sceneGraph;
	SelectionManagerPtr selectionMgr = core::make_shared<SelectionManager>();
	ModifierVolumeWrapper wrapper(node, brush.modifierType(), selectionMgr);

	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));
	brush.preExecute(brushContext, wrapper.volume());
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()))
		<< wrapper.dirtyRegion().toString();

	brush.shutdown();
}

} // namespace voxedit
