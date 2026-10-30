/**
 * @file
 */

#include "voxedit-util/modifier/BrushApply.h"
#include "AbstractSceneManagerTest.h"
#include "color/RGBA.h"
#include "core/Common.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/PaintBrush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

namespace {

class FailAfterWriteBrush : public ShapeBrush {
public:
	bool executeSample(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
					   const BrushContext &ctx) override {
		AABBBrush::executeSample(sceneGraph, wrapper, ctx);
		return false;
	}
};

void fillGround(voxel::RawVolume &volume, uint8_t color) {
	const voxel::Region &region = volume.region();
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			volume.setVoxel(x, region.getLowerY(), z, voxel::Voxel(voxel::VoxelType::Generic, color));
		}
	}
}

int addModel(scenegraph::SceneGraph &sceneGraph, voxel::RawVolume *volume, uint8_t colorA, uint8_t colorB) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	palette::Palette &palette = node.palette();
	palette.setColor(colorA, color::RGBA(40, 40, 40));
	palette.setColor(colorB, color::RGBA(200, 200, 200));
	palette.setSize(core_max(colorA, colorB) + 1);
	return sceneGraph.emplace(core::move(node));
}

BrushSample placeSample(const glm::ivec3 &position, uint8_t color = 1) {
	BrushSample sample;
	sample.position = position;
	sample.face = voxel::FaceNames::PositiveY;
	sample.operation = ModifierType::Place;
	sample.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
	return sample;
}

BrushSample paintSample(const glm::ivec3 &position, uint8_t color = 1) {
	BrushSample sample;
	sample.position = position;
	sample.face = voxel::FaceNames::PositiveY;
	sample.operation = ModifierType::Paint;
	sample.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
	return sample;
}

BrushApplyTarget targetOf(scenegraph::SceneGraphNode &node) {
	BrushApplyTarget target;
	target.node = &node;
	target.volume = node.volume();
	target.writeVolume = node.volume();
	return target;
}

} // namespace

class BrushApplyTest : public app::AbstractTest {};

TEST_F(BrushApplyTest, testSameSampleIndependentTargets) {
	ShapeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();

	scenegraph::SceneGraph sceneGraph;
	const int firstId = addModel(sceneGraph, new voxel::RawVolume(voxel::Region(0, 0, 0, 8, 2, 2)), 0, 1);
	const int secondId = addModel(sceneGraph, new voxel::RawVolume(voxel::Region(0, 0, 0, 8, 2, 2)), 0, 1);
	ASSERT_NE(InvalidNodeId, firstId);
	ASSERT_NE(InvalidNodeId, secondId);

	const BrushSample sample = placeSample(glm::ivec3(0));
	BrushContext settings;
	settings.cursorPosition = glm::ivec3(7, 0, 0);
	settings.modifierType = ModifierType::Erase;

	BrushApplyTarget first = targetOf(sceneGraph.node(firstId));
	BrushApplyTarget second = targetOf(sceneGraph.node(secondId));
	const BrushApplyResult firstResult = applyBrushSample(brush, sceneGraph, sample, first, settings);
	const BrushApplyResult secondResult = applyBrushSample(brush, sceneGraph, sample, second, settings);

	EXPECT_EQ(BrushApplyStatus::Changed, firstResult.status);
	EXPECT_EQ(BrushApplyStatus::Changed, secondResult.status);
	EXPECT_TRUE(firstResult.dirtyRegion.isValid());
	EXPECT_TRUE(secondResult.dirtyRegion.isValid());
	EXPECT_FALSE(voxel::isAir(sceneGraph.node(firstId).volume()->voxel(0, 0, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(sceneGraph.node(secondId).volume()->voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(sceneGraph.node(firstId).volume()->voxel(1, 0, 0).getMaterial()));
	EXPECT_EQ(glm::ivec3(7, 0, 0), settings.cursorPosition);
	EXPECT_EQ(ModifierType::Erase, settings.modifierType);

	brush.shutdown();
}

TEST_F(BrushApplyTest, testPaintRuntimeIndependentPerTarget) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Blend);
	brush.setBlendOpacity(0.5f);

	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *firstVolume = new voxel::RawVolume(voxel::Region(0, 0, 0, 8, 2, 2));
	voxel::RawVolume *secondVolume = new voxel::RawVolume(voxel::Region(0, 0, 0, 8, 2, 2));
	fillGround(*firstVolume, 0);
	fillGround(*secondVolume, 0);
	const int firstId = addModel(sceneGraph, firstVolume, 0, 1);
	const int secondId = addModel(sceneGraph, secondVolume, 0, 1);

	PaintBrush firstBrush(brush);
	PaintBrush secondBrush(brush);
	BrushApplyTarget first = targetOf(sceneGraph.node(firstId));
	BrushApplyTarget second = targetOf(sceneGraph.node(secondId));
	BrushContext settings;

	ASSERT_EQ(BrushApplyStatus::Changed,
			  applyBrushSample(firstBrush, sceneGraph, paintSample(glm::ivec3(0)), first, settings).status);
	ASSERT_EQ(BrushApplyStatus::Changed,
			  applyBrushSample(secondBrush, sceneGraph, paintSample(glm::ivec3(0)), second, settings).status);
	ASSERT_EQ(BrushApplyStatus::Changed,
			  applyBrushSample(firstBrush, sceneGraph, paintSample(glm::ivec3(2, 0, 0)), first, settings).status);

	EXPECT_EQ(2u, firstBrush.runtime().blendStroke.size());
	EXPECT_EQ(1u, secondBrush.runtime().blendStroke.size());
	EXPECT_FALSE(sceneGraph.node(firstId).volume()->voxel(2, 0, 0) ==
				 sceneGraph.node(secondId).volume()->voxel(2, 0, 0));

	brush.shutdown();
}

TEST_F(BrushApplyTest, testUnchangedReportsNoDirtyRegion) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Replace);

	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, 4, 2, 2));
	fillGround(*volume, 1);
	const int nodeId = addModel(sceneGraph, volume, 0, 1);
	BrushApplyTarget target = targetOf(sceneGraph.node(nodeId));
	BrushContext settings;

	const BrushApplyResult result = applyBrushSample(brush, sceneGraph, paintSample(glm::ivec3(0), 1), target, settings);
	EXPECT_EQ(BrushApplyStatus::Unchanged, result.status);
	EXPECT_FALSE(result.dirtyRegion.isValid());
	EXPECT_EQ(1, sceneGraph.node(nodeId).volume()->voxel(0, 0, 0).getColor());

	brush.shutdown();
}

TEST_F(BrushApplyTest, testInvalidTargetBeforeWrites) {
	ShapeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = addModel(sceneGraph, new voxel::RawVolume(voxel::Region(0, 0, 0, 4, 2, 2)), 0, 1);
	BrushApplyTarget target = targetOf(sceneGraph.node(nodeId));
	voxel::RawVolume replacement(voxel::Region(0, 0, 0, 4, 2, 2));
	target.volume = &replacement;
	BrushContext settings;

	const BrushApplyResult result = applyBrushSample(brush, sceneGraph, placeSample(glm::ivec3(0)), target, settings);
	EXPECT_EQ(BrushApplyStatus::InvalidTarget, result.status);
	EXPECT_FALSE(result.dirtyRegion.isValid());
	EXPECT_TRUE(voxel::isAir(sceneGraph.node(nodeId).volume()->voxel(0, 0, 0).getMaterial()));

	BrushApplyTarget missing;
	EXPECT_EQ(BrushApplyStatus::InvalidTarget,
			  applyBrushSample(brush, sceneGraph, placeSample(glm::ivec3(0)), missing, settings).status);

	brush.shutdown();
}

TEST_F(BrushApplyTest, testFailureKeepsPartialWrites) {
	FailAfterWriteBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();

	scenegraph::SceneGraph sceneGraph;
	const int nodeId = addModel(sceneGraph, new voxel::RawVolume(voxel::Region(0, 0, 0, 4, 2, 2)), 0, 1);
	BrushApplyTarget target = targetOf(sceneGraph.node(nodeId));
	BrushContext settings;

	const BrushApplyResult result = applyBrushSample(brush, sceneGraph, placeSample(glm::ivec3(0)), target, settings);
	EXPECT_EQ(BrushApplyStatus::Failed, result.status);
	EXPECT_TRUE(result.dirtyRegion.isValid());
	EXPECT_FALSE(voxel::isAir(sceneGraph.node(nodeId).volume()->voxel(0, 0, 0).getMaterial()));

	brush.shutdown();
}

class BrushApplySceneTest : public AbstractSceneManagerTest {
protected:
	void preparePaintGround() {
		ASSERT_TRUE(_sceneMgr->newScene(true, "apply", voxel::Region(0, 0, 0, 8, 2, 2)));
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
		ASSERT_NE(nullptr, node);
		node->palette().setColor(0, color::RGBA(40, 40, 40));
		node->palette().setColor(1, color::RGBA(200, 200, 200));
		node->palette().setSize(2);
		fillGround(*node->volume(), 0);
	}
};

TEST_F(BrushApplySceneTest, testUnchangedPreservesRedo) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "noop", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	modifier.setModifierType(ModifierType::Override);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());
	modifier.finishEdit();
	modifier.setCursorPosition(glm::ivec3(2, 0, 0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());
	modifier.finishEdit();
	ASSERT_TRUE(_sceneMgr->undo());
	ASSERT_TRUE(_sceneMgr->mementoHandler().canRedo());

	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Replace);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	BrushApplyTarget target = targetOf(*node);
	const BrushApplyResult result = modifier.evaluateSample(modifier.currentEditSample(), target, false);
	EXPECT_EQ(BrushApplyStatus::Unchanged, result.status);
	EXPECT_TRUE(_sceneMgr->mementoHandler().canRedo());
}

TEST_F(BrushApplySceneTest, testPreviewDoesNotTouchCommittedState) {
	preparePaintGround();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Blend);
	modifier.paintBrush().setBlendOpacity(0.5f);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());

	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	const voxel::Voxel committed = node->volume()->voxel(0, 0, 0);
	const size_t states = _sceneMgr->mementoHandler().stateSize();
	const glm::ivec3 liveCursor = modifier.brushContext().cursorPosition;
	const uint8_t paletteSize = node->palette().colorCount();

	voxel::RawVolume scratch(*node->volume());
	scenegraph::SceneGraphNode scratchNode(scenegraph::SceneGraphNodeType::Model);
	scratchNode.setUnownedVolume(&scratch);
	scratchNode.setPalette(node->palette());
	BrushApplyTarget previewTarget;
	previewTarget.node = &scratchNode;
	previewTarget.volume = &scratch;
	previewTarget.writeVolume = &scratch;

	for (int i = 0; i < 3; ++i) {
		BrushSample sample = modifier.currentEditSample();
		sample.position = glm::ivec3(4, 0, 0);
		const BrushApplyResult preview = modifier.evaluateSample(sample, previewTarget, true);
		EXPECT_TRUE(preview.status == BrushApplyStatus::Changed || preview.status == BrushApplyStatus::Unchanged);
	}

	EXPECT_EQ(committed, node->volume()->voxel(0, 0, 0));
	EXPECT_EQ(0, node->volume()->voxel(4, 0, 0).getColor());
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
	EXPECT_EQ(liveCursor, modifier.brushContext().cursorPosition);
	EXPECT_EQ(paletteSize, node->palette().colorCount());

	modifier.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.updateEdit());
	modifier.finishEdit();
	EXPECT_NE(0, node->volume()->voxel(4, 0, 0).getColor());
}

TEST_F(BrushApplySceneTest, testPreviewBetweenSamplesMatchesCommittedOnly) {
	preparePaintGround();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Replace);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));

	auto committedStroke = [&]() {
		modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
		ASSERT_TRUE(modifier.beginEdit());
		ASSERT_TRUE(modifier.updateEdit());
		modifier.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
		ASSERT_TRUE(modifier.updateEdit());
		modifier.finishEdit();
	};

	committedStroke();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	const voxel::Voxel at0 = node->volume()->voxel(0, 0, 0);
	const voxel::Voxel at4 = node->volume()->voxel(4, 0, 0);
	const size_t states = _sceneMgr->mementoHandler().stateSize();

	preparePaintGround();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Replace);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());

	scenegraph::SceneGraphNode *live = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, live);
	voxel::RawVolume scratch(*live->volume());
	scenegraph::SceneGraphNode scratchNode(scenegraph::SceneGraphNodeType::Model);
	scratchNode.setUnownedVolume(&scratch);
	scratchNode.setPalette(live->palette());
	BrushApplyTarget previewTarget = targetOf(scratchNode);
	for (int x = 0; x <= 4; ++x) {
		BrushSample sample = modifier.currentEditSample();
		sample.position = glm::ivec3(x, 0, 0);
		modifier.evaluateSample(sample, previewTarget, true);
	}
	modifier.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.updateEdit());
	modifier.finishEdit();

	EXPECT_EQ(at0, live->volume()->voxel(0, 0, 0));
	EXPECT_EQ(at4, live->volume()->voxel(4, 0, 0));
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
}

TEST_F(BrushApplyTest, testRandomPreviewDoesNotAdvanceTargetRandomState) {
	PaintBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setStrokeMode();
	brush.setPaintMode(PaintBrush::PaintMode::Random);
	scenegraph::SceneGraph scene;
	auto *firstVolume = new voxel::RawVolume(voxel::Region(0, 0, 0, 8, 2, 2));
	fillGround(*firstVolume, 0);
	auto *secondVolume = new voxel::RawVolume(*firstVolume);
	const int firstId = addModel(scene, firstVolume, 0, 1);
	const int secondId = addModel(scene, secondVolume, 0, 1);
	PaintBrush firstBrush(brush);
	PaintBrush secondBrush(brush);
	firstBrush.seedRuntime(42);
	secondBrush.seedRuntime(42);
	BrushApplyTarget first = targetOf(scene.node(firstId));
	BrushApplyTarget second = targetOf(scene.node(secondId));
	BrushContext settings;
	for (int x = 0; x < 8; ++x) {
		const BrushSample sample = paintSample(glm::ivec3(x, 0, 0));
		applyBrushSample(firstBrush, scene, sample, first, settings);
		voxel::RawVolume scratch(*secondVolume);
		scenegraph::SceneGraphNode scratchNode(scenegraph::SceneGraphNodeType::Model);
		scratchNode.setUnownedVolume(&scratch);
		scratchNode.setPalette(scene.node(secondId).palette());
		BrushApplyTarget preview = targetOf(scratchNode);
		BrushApplyOptions options;
		options.preview = true;
		for (int repeat = 0; repeat < 3; ++repeat) {
			PaintBrush previewBrush(secondBrush);
			applyBrushSample(previewBrush, scene, sample, preview, settings, options);
		}
		applyBrushSample(secondBrush, scene, sample, second, settings);
		EXPECT_EQ(firstVolume->voxel(x, 0, 0), secondVolume->voxel(x, 0, 0));
	}
	EXPECT_EQ(firstBrush.runtime().random.random(), secondBrush.runtime().random.random());
	brush.shutdown();
}

TEST_F(BrushApplySceneTest, testProductionPreviewOwnsBlendPalette) {
	preparePaintGround();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Blend);
	modifier.paintBrush().setBlendOpacity(0.5f);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	auto *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	const int colors = node->palette().colorCount();
	modifier.previewManager().updateBrushVolumePreview(modifier, node->palette(), node->volume(),
													   _sceneMgr->sceneGraph());
	const auto *preview = modifier.previewManager().previewVolume();
	ASSERT_NE(nullptr, preview);
	EXPECT_EQ(colors, (int)node->palette().colorCount());
	const uint8_t index = preview->voxel(0, 0, 0).getColor();
	EXPECT_EQ(color::RGBA(120, 120, 120), modifier.previewManager().previewPalette()->color(index));
	EXPECT_EQ(0, (int)node->volume()->voxel(0, 0, 0).getColor());
}

namespace {
class RawFillBrush : public Brush {
protected:
	void generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &input,
				  const voxel::Region &) override {
		wrapper.volume()->fill(input.cursorVoxel);
		wrapper.addToDirtyRegion(wrapper.region().getLowerCorner());
		wrapper.addToDirtyRegion(wrapper.region().getUpperCorner());
	}

public:
	RawFillBrush() : Brush(BrushType::None) {
	}
	voxel::Region calcRegion(const BrushContext &input) const override {
		return input.targetVolumeRegion;
	}
};
} // namespace

TEST_F(BrushApplyTest, testSliceConstrainsAlgorithmsThatWriteTheRawVolume) {
	scenegraph::SceneGraph scene;
	const int id = addModel(scene, new voxel::RawVolume(voxel::Region(-1, 1)), 0, 1);
	auto &node = scene.node(id);
	auto target = targetOf(node);
	target.sliceRegion = voxel::Region(-1, 0, -1, 1, 0, 1);
	RawFillBrush brush;
	BrushContext settings;
	const auto result = applyBrushSample(brush, scene, placeSample(glm::ivec3(0)), target, settings);
	ASSERT_TRUE(result.changed());
	EXPECT_EQ(target.sliceRegion, result.dirtyRegion);
	for (int y = -1; y <= 1; ++y) {
		EXPECT_EQ(y != 0, voxel::isAir(node.volume()->voxel(0, y, 0).getMaterial()));
	}
}

TEST_F(BrushApplyTest, testPaintHasStableVoxelOrder) {
	for (int mode = 0; mode < (int)PaintBrush::PaintMode::Max; ++mode) {
		scenegraph::SceneGraph scene;
		voxel::Voxel original = voxel::createVoxel(voxel::VoxelType::Generic, 0);
		auto *volume = new voxel::RawVolume(voxel::Region(0, 15));
		volume->fill(original);
		const int firstId = addModel(scene, volume, 0, 1);
		const int secondId = addModel(scene, new voxel::RawVolume(*volume), 0, 1);
		PaintBrush first;
		first.setBoxMode();
		first.setPaintMode((PaintBrush::PaintMode)mode);
		first.seedRuntime(42);
		PaintBrush second(first);
		BrushContext input;
		input.evaluatedRegion = volume->region();
		auto firstTarget = targetOf(scene.node(firstId));
		auto secondTarget = targetOf(scene.node(secondId));
		const auto sample = paintSample(glm::ivec3(0));
		ASSERT_FALSE(applyBrushSample(first, scene, sample, firstTarget, input).failed());
		ASSERT_FALSE(applyBrushSample(second, scene, sample, secondTarget, input).failed());
		bool same = true;
		voxelutil::visitVolume(*volume, [&](int x, int y, int z, const voxel::Voxel &value) {
			same &= value == scene.node(secondId).volume()->voxel(x, y, z);
		});
		EXPECT_TRUE(same) << mode;
		EXPECT_EQ(scene.node(firstId).palette().hash(), scene.node(secondId).palette().hash());
		EXPECT_EQ(first.runtime().random.random(), second.runtime().random.random());
	}
}

} // namespace voxedit
