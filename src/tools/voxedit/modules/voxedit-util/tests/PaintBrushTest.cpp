/**
 * @file
 */

#include "voxedit-util/modifier/brush/PaintBrush.h"
#include "app/tests/AbstractTest.h"
#include "color/RGBA.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

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
		node.setVolume(volume);
		palette::Palette &pal = node.palette();
		pal.setColor(existingVoxelColorIndex, color::RGBA(0, 0, 0));
		pal.setColor(paintColorIndex, color::RGBA(255, 255, 255));
		return sceneGraph.emplace(core::move(node));
	}

	void prepareBrushContext(BrushContext &brushContext) {
		brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, paintColorIndex);
	}
};

TEST_F(PaintBrushTest, testExecuteSingle) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();

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
	brush.setStrokeMode();
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

TEST_F(PaintBrushTest, testBlendFalloff) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(1.0f);
	brush.setRadius(5);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	const palette::Palette &palette = node.palette();

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	const voxel::Voxel centerVoxel = wrapper.voxel(brushContext.cursorPosition);
	EXPECT_EQ((int)centerVoxel.getColor(), (int)paintColorIndex) << "Center should be fully painted";
	EXPECT_EQ((int)centerVoxel.getNormal(), (int)existingNormalIndex);

	const glm::ivec3 edgePos = brushContext.cursorPosition + glm::ivec3(5, 0, 0);
	const voxel::Voxel edgeVoxel = wrapper.voxel(edgePos);
	EXPECT_EQ((int)edgeVoxel.getColor(), (int)existingVoxelColorIndex)
		<< "Voxel at exact radius should remain unchanged (smoothstep edge is 0)";

	const glm::ivec3 midPos = brushContext.cursorPosition + glm::ivec3(3, 0, 0);
	const voxel::Voxel midVoxel = wrapper.voxel(midPos);
	const color::RGBA midColor = palette.color(midVoxel.getColor());
	EXPECT_NE((int)midVoxel.getColor(), (int)existingVoxelColorIndex);
	EXPECT_NE((int)midVoxel.getColor(), (int)paintColorIndex);
	EXPECT_GT((int)midColor.r, 0);
	EXPECT_LT((int)midColor.r, 255);

	const glm::ivec3 outsidePos = brushContext.cursorPosition + glm::ivec3(6, 0, 0);
	const voxel::Voxel outsideVoxel = wrapper.voxel(outsidePos);
	EXPECT_EQ((int)outsideVoxel.getColor(), (int)existingVoxelColorIndex);

	brush.endBrush(brushContext);
	brush.shutdown();
}

TEST_F(PaintBrushTest, testBlendOncePerStroke) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(0.5f);
	brush.setRadius(0);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	const palette::Palette &palette = node.palette();

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	const voxel::Voxel first = wrapper.voxel(brushContext.cursorPosition);
	const color::RGBA firstColor = palette.color(first.getColor());
	EXPECT_NE((int)first.getColor(), (int)existingVoxelColorIndex);
	EXPECT_NE((int)first.getColor(), (int)paintColorIndex);

	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	const voxel::Voxel second = wrapper.voxel(brushContext.cursorPosition);
	const color::RGBA secondColor = palette.color(second.getColor());
	EXPECT_EQ((int)second.getColor(), (int)first.getColor()) << "Equal-weight second dab must not restack";
	EXPECT_EQ(secondColor.r, firstColor.r);

	brush.endBrush(brushContext);

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	const voxel::Voxel third = wrapper.voxel(brushContext.cursorPosition);
	EXPECT_NE((int)third.getColor(), (int)first.getColor()) << "A new stroke may blend again";

	brush.endBrush(brushContext);
	brush.shutdown();
}

TEST_F(PaintBrushTest, testBlendStrokeUpgradesWeakEdge) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(1.0f);
	brush.setRadius(5);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	const palette::Palette &palette = node.palette();

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();
	const glm::ivec3 target = brushContext.cursorPosition + glm::ivec3(3, 0, 0);

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	const voxel::Voxel edgeFirst = wrapper.voxel(target);
	const color::RGBA edgeFirstColor = palette.color(edgeFirst.getColor());
	EXPECT_NE((int)edgeFirst.getColor(), (int)existingVoxelColorIndex);
	EXPECT_NE((int)edgeFirst.getColor(), (int)paintColorIndex) << "Target starts as a soft mid-tone";

	// Move the brush center onto the previously weak voxel - max-weight should upgrade
	// from the original color, matching a fresh dab at that center.
	brushContext.cursorPosition = target;
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	const voxel::Voxel upgraded = wrapper.voxel(target);
	EXPECT_EQ((int)upgraded.getColor(), (int)paintColorIndex)
		<< "Stronger coverage must upgrade from the original color, not keep the weak edge blend";
	EXPECT_GT((int)palette.color(upgraded.getColor()).r, (int)edgeFirstColor.r);

	brush.endBrush(brushContext);
	brush.shutdown();
}

TEST_F(PaintBrushTest, testBlendStrokePath) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(1.0f);
	brush.setRadius(1);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	ModifierVolumeWrapper wrapper(sceneGraph.node(nodeId), brush.modifierType());

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	// Jump several voxels away - Bresenham should fill the path between dabs
	brushContext.cursorPosition.x += 4;
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	int painted = 0;
	for (int x = wrapper.region().getLowerCenter().x; x <= brushContext.cursorPosition.x; ++x) {
		const voxel::Voxel v =
			wrapper.voxel(x, brushContext.cursorPosition.y, brushContext.cursorPosition.z);
		if (v.getColor() != existingVoxelColorIndex) {
			++painted;
		}
	}
	EXPECT_GE(painted, 4) << "Stroke path should paint along the line between cursor positions";

	brush.endBrush(brushContext);
	const voxel::Region undoRegion = brush.consumePendingUndoRegion();
	EXPECT_TRUE(undoRegion.isValid()) << "Stroke should produce one pending undo region";
	brush.shutdown();
}

TEST_F(PaintBrushTest, testBlurUsesNeighborColors) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blur);
	brush.setBlendOpacity(1.0f);
	brush.setRadius(0);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	// Checker pattern of black and white on the ground plane
	palette::Palette &pal = node.palette();
	pal.setColor(0, color::RGBA(0, 0, 0));
	pal.setColor(1, color::RGBA(255, 255, 255));
	voxel::RawVolume *volume = node.volume();
	const glm::ivec3 center = volume->region().getLowerCenter();
	volume->setVoxel(center, voxel::Voxel(voxel::VoxelType::Generic, 0, existingNormalIndex));
	volume->setVoxel(center + glm::ivec3(1, 0, 0), voxel::Voxel(voxel::VoxelType::Generic, 1, existingNormalIndex));
	volume->setVoxel(center + glm::ivec3(-1, 0, 0), voxel::Voxel(voxel::VoxelType::Generic, 1, existingNormalIndex));

	ModifierVolumeWrapper wrapper(node, brush.modifierType());
	BrushContext brushContext;
	brushContext.cursorVoxel = voxel::Voxel(voxel::VoxelType::Generic, 1);
	brushContext.cursorPosition = center;

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	brush.endBrush(brushContext);

	const voxel::Voxel blurred = wrapper.voxel(center);
	const color::RGBA c = pal.color(blurred.getColor());
	EXPECT_NE((int)blurred.getColor(), 0) << "Blur should move black center toward white neighbors";
	EXPECT_GT((int)c.r, 0);
	EXPECT_LT((int)c.r, 255);
	brush.shutdown();
}

TEST_F(PaintBrushTest, testBlendRadiusZeroOpacity) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(1.0f);
	brush.setRadius(0);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	ModifierVolumeWrapper wrapper(sceneGraph.node(nodeId), brush.modifierType());

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();

	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	EXPECT_EQ((int)wrapper.voxel(brushContext.cursorPosition).getColor(), (int)paintColorIndex);

	brush.shutdown();
}

TEST_F(PaintBrushTest, testBlendPreviewDoesNotBlockContinuousStroke) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(1.0f);
	brush.setRadius(0);

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = prepareSceneGraph(sceneGraph);
	ASSERT_NE(nodeId, InvalidNodeId);
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	ModifierVolumeWrapper wrapper(node, brush.modifierType());

	BrushContext brushContext;
	prepareBrushContext(brushContext);
	brushContext.cursorPosition = wrapper.region().getLowerCenter();

	ASSERT_TRUE(brush.beginBrush(brushContext));
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	const uint8_t firstColor = wrapper.voxel(brushContext.cursorPosition).getColor();
	EXPECT_NE((int)firstColor, (int)existingVoxelColorIndex);

	// Simulate hover preview while the stroke is still active: must not mark
	// visited voxels or advance the stroke path on the real volume.
	BrushContext previewCtx = brushContext;
	previewCtx.preview = true;
	voxel::RawVolume previewVolume(*node.volume());
	scenegraph::SceneGraphNode previewNode(scenegraph::SceneGraphNodeType::Model);
	previewNode.setUnownedVolume(&previewVolume);
	previewNode.setPalette(node.palette());
	ModifierVolumeWrapper previewWrapper(previewNode, brush.modifierType());
	ASSERT_TRUE(brush.execute(sceneGraph, previewWrapper, previewCtx));

	brushContext.cursorPosition.x += 3;
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));

	int painted = 0;
	for (int x = wrapper.region().getLowerCenter().x; x <= brushContext.cursorPosition.x; ++x) {
		if (wrapper.voxel(x, brushContext.cursorPosition.y, brushContext.cursorPosition.z).getColor() !=
			existingVoxelColorIndex) {
			++painted;
		}
	}
	EXPECT_GE(painted, 3) << "Continuous stroke must keep painting after preview executes";

	brush.endBrush(brushContext);
	brush.shutdown();
}

} // namespace voxedit
