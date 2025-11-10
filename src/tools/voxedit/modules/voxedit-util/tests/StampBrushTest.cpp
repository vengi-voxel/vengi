/**
 * @file
 */

#include "voxedit-util/modifier/brush/StampBrush.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxedit {

class StampBrushTest : public app::AbstractTest {
protected:
};

TEST_F(StampBrushTest, testExecute) {
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>(),
					 core::make_shared<SelectionManager>());
	StampBrush brush(&mgr);
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(-3, 3));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	scenegraph::SceneGraph sceneGraph;
	SelectionManagerPtr selectionMgr = core::make_shared<SelectionManager>();
	ModifierVolumeWrapper wrapper(node, brush.modifierType(), selectionMgr);
	palette::Palette palette;
	palette.nippon();

	EXPECT_FALSE(brush.active());
	brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0), palette);
	EXPECT_TRUE(brush.active());

	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));
	brush.preExecute(brushContext, wrapper.volume());
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	brush.postExecute(brushContext);
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial())) << wrapper.dirtyRegion().toString();

	brush.shutdown();
}

} // namespace voxedit
