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
	brushContext.gridResolution = glm::ivec3(1);

	brush.setBezier(true);
	EXPECT_TRUE(brush.hasHandle(brushContext));

	BrushGizmoState state;
	brush.describeHandle(brushContext, state);
	EXPECT_EQ(BrushGizmo_BezierControl | BrushGizmo_Line, state.operations);
	EXPECT_EQ(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(state.matrix[3]));
	EXPECT_EQ(3, state.numPositions);

	const glm::ivec3 controlPoint(5, 5, 0);
	glm::mat4 matrix(1.0f);
	matrix[3] = glm::vec4(controlPoint, 1.0f);

	ASSERT_TRUE(brush.beginBrush(brushContext));
	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, previewWrapper, brushContext));
	brush.endBrush(brushContext);
	EXPECT_EQ(glm::ivec3(10, 0, 0), brushContext.referencePos);
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(glm::ivec3(5, 3, 0)).getMaterial()));

	EXPECT_TRUE(brush.editHandle(0, brushContext, matrix, glm::mat4(1.0f), BrushGizmo_Translate));
	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, previewWrapper, brushContext));

	const math::Bezier<int> bezier(brushContext.referencePos, brushContext.cursorPosition, controlPoint);
	const math::Bezier<int> committedBezier(glm::ivec3(0), glm::ivec3(10, 0, 0), controlPoint);
	const glm::ivec3 expectedMid = committedBezier.getPoint(0.5f);
	EXPECT_TRUE(voxel::isBlocked(previewWrapper.voxel(expectedMid).getMaterial())) << previewWrapper.dirtyRegion().toString();
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(expectedMid).getMaterial()));

	EXPECT_EQ(1, brush.pendingSegmentCount());
	ASSERT_TRUE(brush.beginBrush(brushContext));
	brush.preExecute(brushContext, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	brush.endBrush(brushContext);
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(expectedMid).getMaterial())) << wrapper.dirtyRegion().toString();
	brush.resetRuntime();
	EXPECT_EQ(0, brush.pendingSegmentCount());

	brush.shutdown();
}

TEST_F(LineBrushTest, testBezierNoFlushOnCursorMove) {
	// Regression test for #860: moving the cursor in bezier mode should not
	// execute the brush on the real volume or add segments every frame.
	LineBrush brush;

	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(glm::ivec3(0), glm::ivec3(10, 6, 1)));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	scenegraph::SceneGraph sceneGraph;
	BrushContext brushContext;
	brushContext.referencePos = glm::ivec3(0);
	brushContext.cursorPosition = glm::ivec3(5, 0, 0);
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, 0);
	brushContext.gridResolution = glm::ivec3(1);

	brush.setBezier(true);

	// Simulate first click: begin + execute on real volume to lock a segment
	ASSERT_TRUE(brush.beginBrush(brushContext));
	brush.preExecute(brushContext, &volume);
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	EXPECT_EQ(1, brush.pendingSegmentCount());
	brush.endBrush(brushContext);
	EXPECT_EQ(1, brush.pendingSegmentCount());
	EXPECT_EQ(1, brush.pendingSegmentCount());

	// Pending bezier segments must not write the real volume until the line is executed.
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));

	// Simulate cursor movement: update should mark dirty but must NOT add segments
	brushContext.cursorPosition = glm::ivec3(8, 0, 0);
	brush.update(brushContext, 1.0);
	EXPECT_EQ(1, brush.pendingSegmentCount()) << "cursor move must not add segments";

	// Another begin/end on the real volume should add only the intentional click,
	// not a segment for every cursor move.
	brushContext.cursorPosition = glm::ivec3(10, 0, 0);
	ASSERT_TRUE(brush.beginBrush(brushContext));
	brush.preExecute(brushContext, &volume);
	ModifierVolumeWrapper wrapper2(node, brush.modifierType());
	EXPECT_EQ(2, brush.pendingSegmentCount());
	brush.endBrush(brushContext);
	EXPECT_EQ(2, brush.pendingSegmentCount()) << "only explicit clicks should add segments";

	// The real volume should still be empty - segments are only committed on deactivation
	EXPECT_FALSE(voxel::isBlocked(volume.voxel(glm::ivec3(5, 0, 0)).getMaterial()));

	brush.shutdown();
}

TEST_P(BrushTestParamTest, testLineBrush) {
	LineBrush brush;
	testPlaceAndOverride(brush);
}

TEST_F(LineBrushTest, testSag) {
	LineBrush brush;

	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(glm::ivec3(0), glm::ivec3(10, 10, 1)));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	BrushContext brushContext;
	brushContext.referencePos = glm::ivec3(0, 5, 0);
	brushContext.cursorPosition = glm::ivec3(10, 5, 0);
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, 0);

	brush.setSag(3);
	EXPECT_EQ(3, brush.sag());

	brush.preExecute(brushContext, nullptr);
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	// The midpoint should be displaced downward by the sag amount
	// midpoint is (5, 5, 0), control is (5, 2, 0), bezier midpoint at t=0.5 is (5, 3.5, 0)
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.referencePos).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));
	// Check that a voxel below the straight line was placed (sag effect)
	const glm::ivec3 belowMid(5, 4, 0);
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(belowMid).getMaterial())) << "sag should place voxels below the straight line";

	brush.shutdown();
}

TEST_F(LineBrushTest, testHandleIdentitySurvivesRemovingAnEarlierSegment) {
	LineBrush brush;
	brush.setBezier(true);
	BrushContext input;
	input.cursorPosition = glm::ivec3(3, 0, 0);
	ASSERT_TRUE(brush.beginBrush(input));
	brush.endBrush(input);
	input.cursorPosition.x = 6;
	ASSERT_TRUE(brush.beginBrush(input));
	brush.endBrush(input);
	core::DynamicArray<BrushGizmoState> before;
	brush.handles(input, before);
	ASSERT_EQ(6u, before.size());
	const uint32_t id = before[3].id;
	ASSERT_TRUE(brush.removeSegment(0));
	core::DynamicArray<BrushGizmoState> after;
	brush.handles(input, after);
	ASSERT_EQ(3u, after.size());
	EXPECT_EQ(id, after[0].id);
	glm::mat4 matrix(1);
	matrix[3] = glm::vec4(4, 2, 0, 1);
	ASSERT_TRUE(brush.editHandle(id, input, matrix, glm::mat4(1), BrushGizmo_Translate));
	ASSERT_NE(nullptr, brush.segment(0));
	EXPECT_EQ(glm::ivec3(4, 2, 0), brush.segment(0)->control);
}

} // namespace voxedit
