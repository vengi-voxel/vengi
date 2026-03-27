/**
 * @file
 */

#include "voxedit-util/modifier/brush/LineBrush.h"
#include "app/tests/AbstractTest.h"
#include "math/Bezier.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxedit-util/tests/AbstractBrushTest.h"
#include "voxel/Voxel.h"

namespace voxedit {

class LineBrushTest : public app::AbstractTest {};

TEST_F(LineBrushTest, testExecute) {
	LineBrush brush;

	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(-3, 3));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
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

TEST_F(LineBrushTest, testBezierExecuteAndGizmo) {
	LineBrush brush;

	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(glm::ivec3(0), glm::ivec3(10, 6, 1)));
	voxel::RawVolume previewVolume(volume.region());
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	scenegraph::SceneGraphNode previewNode(scenegraph::SceneGraphNodeType::Model);
	previewNode.setUnownedVolume(&previewVolume);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	ModifierVolumeWrapper previewWrapper(previewNode, brush.modifierType());
	BrushContext brushContext;
	brushContext.referencePos = glm::ivec3(0);
	brushContext.cursorPosition = glm::ivec3(10, 0, 0);
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, 0);
	brushContext.gridResolution = 1;

	brush.setBezier(true);
	EXPECT_TRUE(brush.wantBrushGizmo(brushContext));

	BrushGizmoState state;
	brush.brushGizmoState(brushContext, state);
	EXPECT_EQ(BrushGizmo_BezierControl, state.operations);
	EXPECT_EQ(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(state.matrix[3]));

	const glm::ivec3 controlPoint(5, 5, 0);
	glm::mat4 matrix(1.0f);
	matrix[3] = glm::vec4(controlPoint, 1.0f);

	ASSERT_TRUE(brush.beginBrush(brushContext));
	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	brush.endBrush(brushContext);
	EXPECT_EQ(glm::ivec3(10, 0, 0), brushContext.referencePos);
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(glm::ivec3(5, 3, 0)).getMaterial()));

	EXPECT_TRUE(brush.applyBrushGizmo(brushContext, matrix, glm::mat4(1.0f), BrushGizmo_Translate));
	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, previewWrapper, brushContext));

	const math::Bezier<int> bezier(brushContext.referencePos, brushContext.cursorPosition, controlPoint);
	const math::Bezier<int> committedBezier(glm::ivec3(0), glm::ivec3(10, 0, 0), controlPoint);
	const glm::ivec3 expectedMid = committedBezier.getPoint(0.5f);
	EXPECT_TRUE(voxel::isBlocked(previewWrapper.voxel(expectedMid).getMaterial())) << previewWrapper.dirtyRegion().toString();
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(expectedMid).getMaterial()));

	EXPECT_TRUE(brush.onDeactivated());
	ASSERT_TRUE(brush.beginBrush(brushContext));
	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	brush.endBrush(brushContext);
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(expectedMid).getMaterial())) << wrapper.dirtyRegion().toString();
	EXPECT_FALSE(brush.hasPendingChanges());

	brush.shutdown();
}

TEST_P(BrushTestParamTest, testLineBrush) {
	LineBrush brush;
	testPlaceAndOverride(brush);
}

} // namespace voxedit
