/**
 * @file
 */

#include "voxedit-util/modifier/brush/LineBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/tests/AbstractBrushTest.h"
#include "voxel/Voxel.h"

namespace voxedit {

class LineBrushTest : public app::AbstractTest {};

TEST_F(LineBrushTest, testExecute) {
	LineBrush brush;

	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(-3, 3));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	BrushContext brushContext;
	brushContext.referencePos = volume.region().getLowerCorner();
	brushContext.cursorPosition = volume.region().getUpperCorner();
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, 0);

	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.referencePos).getMaterial()));

	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial())) << wrapper.dirtyRegion().toString();
	// TODO: fails on linux - but not on mac or windows. need to investigate...
	// EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.referencePos).getMaterial())) << wrapper.dirtyRegion().toString();

	brush.shutdown();
}

TEST_P(BrushTestParamTest, testLineBrush) {
	LineBrush brush;
	testPlaceAndOverride(brush);
}

} // namespace voxedit
