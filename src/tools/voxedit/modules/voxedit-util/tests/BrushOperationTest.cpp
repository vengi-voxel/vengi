/**
 * @file
 */

#include "AbstractSceneManagerTest.h"
#include "voxedit-util/modifier/DraftEditSession.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/operation/ShapeOperation.h"
#include "voxedit-util/modifier/brush/operation/TransformOperation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

namespace voxedit {

class BrushOperationTest : public AbstractSceneManagerTest {
protected:
	static voxel::Voxel selected(uint8_t color = 1) {
		voxel::Voxel value = voxel::createVoxel(voxel::VoxelType::Generic, color);
		value.setOutline();
		return value;
	}

	scenegraph::SceneGraphNode &prepare() {
		_sceneMgr->newScene(true, "operation", voxel::Region(-5, 5));
		scenegraph::SceneGraphNode &node = _sceneMgr->sceneGraph().node(_sceneMgr->activeNodeUUID());
		node.volume()->setVoxel(0, 0, 0, selected());
		_sceneMgr->modified(node.uuid(), voxel::Region(0, 0));
		return node;
	}
};

TEST_F(BrushOperationTest, testTransformDraftDoesNotChangeCommittedScene) {
	scenegraph::SceneGraphNode &node = prepare();
	voxel::RawVolume *volume = node.volume();
	const size_t states = _sceneMgr->mementoHandler().stateSize();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	EXPECT_EQ(selected(), volume->voxel(0, 0, 0));
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	const scenegraph::SceneGraphNode *draft = modifier.renderSceneGraph().findNodeByUUID(node.uuid());
	ASSERT_NE(nullptr, draft);
	const voxel::RawVolume *draftVolume = draft->volume();
	EXPECT_TRUE(voxel::isAir(draftVolume->voxel(0, 0, 0).getMaterial()));
	EXPECT_EQ(selected(), draftVolume->voxel(2, 0, 0));
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
	modifier.cancelDraft();
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_EQ(selected(), volume->voxel(0, 0, 0));
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
}

TEST_F(BrushOperationTest, testTransformApplyUndoRedo) {
	scenegraph::SceneGraphNode &node = prepare();
	const core::UUID uuid = node.uuid();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	ASSERT_TRUE(modifier.applyDraft().changed());
	EXPECT_EQ(selected(), node.volume()->voxel(2, 0, 0));
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_EQ(selected(), _sceneMgr->sceneGraphNodeByUUID(uuid)->volume()->voxel(0, 0, 0));
	ASSERT_TRUE(_sceneMgr->redo());
	EXPECT_EQ(selected(), _sceneMgr->sceneGraphNodeByUUID(uuid)->volume()->voxel(2, 0, 0));
}

TEST_F(BrushOperationTest, testSceneReloadDiscardsDraftEvenWithMatchingNodeIdentity) {
	const core::UUID uuid = prepare().uuid();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	scenegraph::SceneGraph replacement;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, uuid);
	node.setVolume(new voxel::RawVolume(voxel::Region(-5, 5)));
	node.volume()->setVoxel(0, 0, 0, selected(3));
	replacement.emplace(core::move(node));
	ASSERT_TRUE(replacement.setActiveNode(uuid));
	ASSERT_TRUE(_sceneMgr->loadSceneGraph(core::move(replacement)));
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_FALSE(modifier.applyDraft().changed());
	const scenegraph::SceneGraphNode *loaded = _sceneMgr->sceneGraphNodeByUUID(uuid);
	ASSERT_NE(nullptr, loaded);
	const voxel::RawVolume *loadedVolume = loaded->volume();
	EXPECT_EQ(selected(3), loadedVolume->voxel(0, 0, 0));
	EXPECT_TRUE(voxel::isAir(loadedVolume->voxel(2, 0, 0).getMaterial()));
}

TEST_F(BrushOperationTest, testNewSceneFinishesStrokeAndDiscardsDraft) {
	prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(_sceneMgr->newScene(true, "stroke replacement", voxel::Region(-5, 5)));
	EXPECT_FALSE(modifier.isEditActive());
	scenegraph::SceneGraphNode &source = _sceneMgr->sceneGraph().node(_sceneMgr->activeNodeUUID());
	source.volume()->setVoxel(0, 0, 0, selected());
	_sceneMgr->modified(source.uuid(), voxel::Region(0, 0));
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	ASSERT_TRUE(_sceneMgr->newScene(true, "draft replacement", voxel::Region(-5, 5)));
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_FALSE(modifier.applyDraft().changed());
	const voxel::RawVolume *volume = _sceneMgr->sceneGraph().node(_sceneMgr->activeNodeUUID()).volume();
	EXPECT_TRUE(volume->isEmpty(volume->region()));
}

TEST_F(BrushOperationTest, testIdentityDraftPreservesRedo) {
	prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	ASSERT_TRUE(modifier.applyDraft().changed());
	ASSERT_TRUE(_sceneMgr->undo());
	ASSERT_TRUE(_sceneMgr->mementoHandler().canRedo());
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	parameters.translation = glm::ivec3(0);
	EXPECT_EQ(BrushApplyStatus::Unchanged, modifier.updateParameters<TransformBrush>("transform", parameters).status);
	EXPECT_EQ(BrushApplyStatus::Unchanged, modifier.applyDraft().status);
	EXPECT_TRUE(_sceneMgr->mementoHandler().canRedo());
	EXPECT_FALSE(modifier.hasDraft());
}

TEST_F(BrushOperationTest, testRepeatedTransformUsesOriginalInput) {
	scenegraph::SceneGraphNode &node = prepare();
	DraftEditSession<TransformOperation> session;
	core::Buffer<core::UUID> targets;
	targets.push_back(node.uuid());
	ASSERT_TRUE(session.begin(_sceneMgr->sceneGraph(), targets));
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(session.update(parameters).changed());
	parameters.translation = glm::ivec3(3, 0, 0);
	ASSERT_TRUE(session.update(parameters).changed());
	const scenegraph::SceneGraphNode *draft = session.view()->findNodeByUUID(node.uuid());
	ASSERT_NE(nullptr, draft);
	const voxel::RawVolume *draftVolume = draft->volume();
	EXPECT_TRUE(voxel::isAir(draftVolume->voxel(2, 0, 0).getMaterial()));
	EXPECT_EQ(selected(), draftVolume->voxel(3, 0, 0));
	EXPECT_EQ(selected(), node.volume()->voxel(0, 0, 0));
}

TEST_F(BrushOperationTest, testExternalMutationRejectsEveryTargetBeforePublication) {
	scenegraph::SceneGraphNode &first = prepare();
	const core::UUID firstUUID = first.uuid();
	const int secondId = _sceneMgr->addModelChild("second", 11, 11, 11);
	scenegraph::SceneGraphNode &second = _sceneMgr->sceneGraph().node(secondId);
	voxel::RawVolume *secondVolume = second.volume();
	secondVolume->setVoxel(0, 0, 0, selected(2));
	core::Buffer<core::UUID> targets;
	targets.push_back(firstUUID);
	targets.push_back(second.uuid());
	DraftEditSession<TransformOperation> session;
	ASSERT_TRUE(session.begin(_sceneMgr->sceneGraph(), targets));
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(session.update(parameters).changed());
	secondVolume->setVoxel(1, 0, 0, selected(3));
	EXPECT_EQ(BrushApplyStatus::InvalidTarget, session.apply(_sceneMgr->sceneGraph()).status);
	EXPECT_EQ(selected(), _sceneMgr->sceneGraphNodeByUUID(firstUUID)->volume()->voxel(0, 0, 0));
	EXPECT_EQ(selected(2), secondVolume->voxel(0, 0, 0));
	EXPECT_EQ(selected(3), secondVolume->voxel(1, 0, 0));
}

TEST_F(BrushOperationTest, testGizmoAndParameterUpdatesAgree) {
	scenegraph::SceneGraphNode &node = prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	BrushGizmoState state;
	core::DynamicArray<BrushGizmoState> handles;
	modifier.brushHandles(handles);
	ASSERT_FALSE(handles.empty());
	state = handles[0];
	glm::mat4 matrix = state.matrix;
	matrix[3].x += 2.0f;
	BrushHandleEvent event;
	event.operationId = state.operationId;
	event.handleId = state.id;
	event.viewportId = 1;
	event.phase = BrushHandlePhase::Begin;
	ASSERT_TRUE(modifier.editBrushHandle(event));
	event.phase = BrushHandlePhase::Change;
	event.matrix = matrix;
	event.operation = BrushGizmo_Translate;
	ASSERT_TRUE(modifier.editBrushHandle(event));
	event.phase = BrushHandlePhase::End;
	modifier.editBrushHandle(event);
	EXPECT_EQ(glm::ivec3(2, 0, 0), modifier.transformBrush().parameters().translation);
	EXPECT_EQ(selected(), modifier.renderSceneGraph().findNodeByUUID(node.uuid())->volume()->voxel(2, 0, 0));
	EXPECT_EQ(selected(), node.volume()->voxel(0, 0, 0));
}

TEST_F(BrushOperationTest, testInvalidParametersKeepCommittedScene) {
	scenegraph::SceneGraphNode &node = prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.scale.x = std::numeric_limits<float>::quiet_NaN();
	EXPECT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).failed());
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_EQ(selected(), node.volume()->voxel(0, 0, 0));
}

TEST_F(BrushOperationTest, testFailedResultStillReportsChanges) {
	BrushApplyResult result;
	BrushApplyResult changed;
	changed.status = BrushApplyStatus::Changed;
	changed.dirtyRegion = voxel::Region(0, 0);
	result.accumulate(changed);
	BrushApplyResult failed;
	failed.status = BrushApplyStatus::Failed;
	result.accumulate(failed);
	EXPECT_TRUE(result.failed());
	EXPECT_TRUE(result.changed());
	EXPECT_EQ(changed.dirtyRegion, result.dirtyRegion);
	result.accumulate(changed);
	EXPECT_TRUE(result.failed());
}

TEST_F(BrushOperationTest, testInvalidDraftCannotPublishPreviousValidPreview) {
	scenegraph::SceneGraphNode &node = prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Transform);
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	parameters.scale.x = std::numeric_limits<float>::quiet_NaN();
	EXPECT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).failed());
	EXPECT_NE(nullptr, modifier.operationError());
	EXPECT_TRUE(modifier.applyDraft().failed());
	EXPECT_EQ(selected(), node.volume()->voxel(0, 0, 0));
	parameters.scale = glm::vec3(1.0f);
	EXPECT_FALSE(modifier.updateParameters<TransformBrush>("transform", parameters).failed());
	EXPECT_EQ(nullptr, modifier.operationError());
	EXPECT_TRUE(modifier.applyDraft().changed());
}

TEST_F(BrushOperationTest, testSliceDraftPreservesOtherPlanes) {
	scenegraph::SceneGraphNode &node = prepare();
	voxel::RawVolume *volume = node.volume();
	volume->setVoxel(0, 1, 0, selected(2));
	DraftEditSession<TransformOperation> session;
	core::Buffer<core::UUID> targets;
	targets.push_back(node.uuid());
	ASSERT_TRUE(session.begin(_sceneMgr->sceneGraph(), targets, voxel::Region(-5, 0, -5, 5, 0, 5)));
	TransformParameters parameters;
	parameters.translation = glm::ivec3(2, 0, 0);
	ASSERT_TRUE(session.update(parameters).changed());
	ASSERT_TRUE(session.apply(_sceneMgr->sceneGraph()).changed());
	EXPECT_EQ(selected(), volume->voxel(2, 0, 0));
	EXPECT_EQ(selected(2), volume->voxel(0, 1, 0));
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 1, 0).getMaterial()));
}

namespace {
struct ExampleParameters {
	glm::ivec3 position{1, 0, 0};
	bool colorOnly = false;
};
class ExampleOperation {
	ExampleParameters _parameters;
	bool _prepared = false;

public:
	using Parameters = ExampleParameters;
	static constexpr bool requiresSelection = false;
	static bool sameOperation(const Parameters &, const Parameters &) {
		return true;
	}
	bool setParameters(const Parameters &parameters) {
		_parameters = parameters;
		return true;
	}
	const Parameters &parameters() const {
		return _parameters;
	}
	void prepare(const voxel::RawVolume *volume, const voxel::Region &) {
		_prepared = volume != nullptr;
	}
	bool hasSnapshot() const {
		return _prepared;
	}
	void evaluate(ModifierVolumeWrapper &wrapper) {
		if (_parameters.colorOnly) {
			wrapper.node().palette().setColor(1, color::RGBA(12, 34, 56));
			return;
		}
		wrapper.setVoxel(_parameters.position.x, _parameters.position.y, _parameters.position.z,
						 voxel::createVoxel(voxel::VoxelType::Generic, 2));
	}
};
class ExampleBrush : public Brush {
protected:
	void generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &, const BrushContext &,
				  const voxel::Region &) override {
	}

public:
	ExampleParameters parameters;
	ExampleBrush() : Brush(BrushType::None) {
	}
	void setParameters(const ExampleParameters &value) {
		markSettingsChanged();
		parameters = value;
	}
	voxel::Region calcRegion(const BrushContext &) const override {
		return voxel::Region(parameters.position, parameters.position);
	}
	void handles(const BrushContext &, core::DynamicArray<BrushGizmoState> &output) const override {
		BrushGizmoState handle;
		handle.id = 42;
		handle.operations = BrushGizmo_Translate;
		handle.matrix[3] = glm::vec4(glm::vec3(parameters.position), 1);
		output.push_back(handle);
	}
	bool editHandle(uint32_t handleId, BrushContext &, const glm::mat4 &matrix, const glm::mat4 &, uint32_t) override {
		parameters.position = glm::ivec3(glm::round(glm::vec3(matrix[3])));
		return true;
	}
};
core::SharedPtr<ExampleBrush> registerExample(Modifier &modifier) {
	core::SharedPtr<ExampleBrush> tool = core::make_shared<ExampleBrush>();
	core::SharedPtr<TypedBrushDraft<ExampleOperation>> draft = core::make_shared<TypedBrushDraft<ExampleOperation>>(
		[tool](const BrushContext &) { return tool->parameters; },
		[tool](const ExampleParameters &parameters) { tool->parameters = parameters; }, []() {});
	modifier.registry().add("example", tool, draft);
	return tool;
}
} // namespace

TEST_F(BrushOperationTest, testRegisteredToolGetsPreviewHandlesPublicationAndUndo) {
	scenegraph::SceneGraphNode &node = prepare();
	voxel::RawVolume *volume = node.volume();
	node.clearSelection();
	const core::UUID uuid = node.uuid();
	Modifier &modifier = _sceneMgr->modifier();
	core::SharedPtr<ExampleBrush> tool = registerExample(modifier);
	ASSERT_TRUE(modifier.setTool("example"));
	ExampleParameters parameters;
	parameters.position = glm::ivec3(2, 0, 0);
	tool->setParameters(parameters);
	modifier.flushPendingBrushChanges();
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	EXPECT_EQ(2, modifier.renderSceneGraph().findNodeByUUID(uuid)->volume()->voxel(2, 0, 0).getColor());
	core::DynamicArray<BrushGizmoState> handles;
	modifier.brushHandles(handles);
	ASSERT_EQ(1u, handles.size());
	BrushHandleEvent event;
	event.operationId = handles[0].operationId;
	event.handleId = 42;
	event.viewportId = 1;
	event.phase = BrushHandlePhase::Begin;
	ASSERT_TRUE(modifier.editBrushHandle(event));
	BrushHandleEvent other = event;
	other.viewportId = 2;
	EXPECT_FALSE(modifier.editBrushHandle(other));
	event.phase = BrushHandlePhase::Change;
	event.matrix[3] = glm::vec4(3, 0, 0, 1);
	ASSERT_TRUE(modifier.editBrushHandle(event));
	EXPECT_EQ(glm::ivec3(3, 0, 0), tool->parameters.position);
	event.phase = BrushHandlePhase::End;
	ASSERT_TRUE(modifier.editBrushHandle(event));
	ASSERT_TRUE(modifier.applyDraft().changed());
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	EXPECT_EQ(2, volume->voxel(3, 0, 0).getColor());
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_TRUE(voxel::isAir(_sceneMgr->sceneGraphNodeByUUID(uuid)->volume()->voxel(3, 0, 0).getMaterial()));
	ASSERT_TRUE(_sceneMgr->redo());
	EXPECT_EQ(2, _sceneMgr->sceneGraphNodeByUUID(uuid)->volume()->voxel(3, 0, 0).getColor());
	modifier.setTool("shape");
	EXPECT_FALSE(modifier.editBrushHandle(event));
}

TEST_F(BrushOperationTest, testPaletteOnlyDraftCancelApplyUndo) {
	scenegraph::SceneGraphNode &node = prepare();
	const core::UUID uuid = node.uuid();
	const color::RGBA original = node.palette().color(1);
	Modifier &modifier = _sceneMgr->modifier();
	core::SharedPtr<ExampleBrush> tool = registerExample(modifier);
	modifier.setTool("example");
	ExampleParameters parameters;
	parameters.colorOnly = true;
	tool->setParameters(parameters);
	modifier.flushPendingBrushChanges();
	EXPECT_EQ(original, node.palette().color(1));
	modifier.cancelDraft();
	EXPECT_EQ(original, node.palette().color(1));
	ASSERT_TRUE(modifier.parametersChanged().paletteChanged);
	const BrushApplyResult result = modifier.applyDraft();
	EXPECT_TRUE(result.paletteChanged);
	EXPECT_TRUE(result.changed());
	EXPECT_EQ(color::RGBA(12, 34, 56), node.palette().color(1));
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_EQ(original, _sceneMgr->sceneGraphNodeByUUID(uuid)->palette().color(1));
}

TEST_F(BrushOperationTest, testHeadlessRequestPreservesToolAndPreferences) {
	scenegraph::SceneGraphNode &node = prepare();
	voxel::RawVolume *volume = node.volume();
	node.clearSelection();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("paint");
	modifier.shapeBrush().setRadius(7);
	ShapeBrush tool;
	tool.setBoxMode();
	BrushContext input;
	input.evaluatedRegion = voxel::Region(2, 0, 0, 3, 0, 0);
	input.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	core::Buffer<core::UUID> targets;
	targets.push_back(node.uuid());
	ASSERT_TRUE(modifier.executeRequest(tool, input, targets).changed());
	EXPECT_EQ(core::String("paint"), modifier.toolId());
	EXPECT_EQ(7, modifier.shapeBrush().radius());
	EXPECT_EQ(2, volume->voxel(2, 0, 0).getColor());
	EXPECT_EQ(2, volume->voxel(3, 0, 0).getColor());
}

TEST_F(BrushOperationTest, testExternalSelectionMutationDiscardsDraftBeforeWriting) {
	scenegraph::SceneGraphNode &node = prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("transform");
	TransformParameters parameters;
	parameters.translation.x = 2;
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	_sceneMgr->selectionUnselect(node.uuid());
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_FALSE(node.hasSelection());
	EXPECT_TRUE(voxel::isAir(node.volume()->voxel(2, 0, 0).getMaterial()));
}

TEST_F(BrushOperationTest, testShapeFieldsAndBoundsHandleUseTheSameRegion) {
	prepare().clearSelection();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("shape");
	modifier.shapeBrush().setBoxMode();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 2));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateBoxRegion(voxel::Region(1, 1, 1, 2, 2, 2)));
	core::DynamicArray<BrushGizmoState> handles;
	modifier.brushHandles(handles);
	ASSERT_EQ(1u, handles.size());
	BrushHandleEvent event;
	event.operationId = handles[0].operationId;
	event.viewportId = 1;
	event.phase = BrushHandlePhase::Begin;
	ASSERT_TRUE(modifier.editBrushHandle(event));
	event.phase = BrushHandlePhase::Change;
	event.matrix = handles[0].matrix;
	event.matrix[3].x += 2;
	ASSERT_TRUE(modifier.editBrushHandle(event));
	EXPECT_EQ(voxel::Region(3, 1, 1, 4, 2, 2), modifier.calcBrushRegion());
	event.phase = BrushHandlePhase::End;
	modifier.editBrushHandle(event);
	modifier.finishEdit(BrushEditFinishReason::Release);
	EXPECT_FALSE(voxel::isAir(
		_sceneMgr->sceneGraphNodeByUUID(_sceneMgr->activeNodeUUID())->volume()->voxel(3, 1, 1).getMaterial()));
}

TEST_F(BrushOperationTest, testSceneScriptDiscardsDraftBeforeWriting) {
	scenegraph::SceneGraphNode &node = prepare();
	voxel::RawVolume *volume = node.volume();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("transform");
	TransformParameters parameters;
	parameters.translation.x = 2;
	ASSERT_TRUE(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
	core::DynamicArray<core::String> arguments;
	ASSERT_TRUE(_sceneMgr->runScriptSync("function main(node, region, color) node:volume():setVoxel(1, 0, 0, 2) end",
										 arguments));
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_EQ(selected(), volume->voxel(0, 0, 0));
	EXPECT_EQ(2, volume->voxel(1, 0, 0).getColor());
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
}

TEST_F(BrushOperationTest, testDirectLineModeChangeAppliesExistingDraft) {
	scenegraph::SceneGraphNode &node = prepare();
	node.clearSelection();
	const voxel::RawVolume *volume = node.volume();
	const core::UUID uuid = node.uuid();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("line");
	LineBrush &brush = modifier.lineBrush();
	brush.setBezier(true);
	modifier.setReferencePosition(glm::ivec3(1, 0, 0));
	modifier.setCursorPosition(glm::ivec3(3, 0, 0), voxel::FaceNames::PositiveY);
	modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 2));
	ASSERT_TRUE(modifier.beginBrush());
	modifier.flushPendingBrushChanges();
	ASSERT_TRUE(modifier.hasDraft());
	EXPECT_TRUE(voxel::isAir(volume->voxel(3, 0, 0).getMaterial()));
	brush.setBezier(false);
	modifier.flushPendingBrushChanges();
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_EQ(2, volume->voxel(3, 0, 0).getColor());
	ASSERT_TRUE(_sceneMgr->undo());
	volume = _sceneMgr->sceneGraphNodeByUUID(uuid)->volume();
	EXPECT_TRUE(voxel::isAir(volume->voxel(3, 0, 0).getMaterial()));
}

TEST_F(BrushOperationTest, testDirectExtrudeSettersRefreshPrivateDraft) {
	scenegraph::SceneGraphNode &node = prepare();
	const voxel::RawVolume *volume = node.volume();
	const core::UUID uuid = node.uuid();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("extrude");
	modifier.setCursorVoxel(selected());
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveX);
	ASSERT_TRUE(modifier.beginBrush());
	ExtrudeBrush &brush = modifier.extrudeBrush();
	brush.setDepth(1);
	modifier.flushPendingBrushChanges();
	ASSERT_TRUE(modifier.hasDraft());
	EXPECT_TRUE(voxel::isAir(volume->voxel(1, 0, 0).getMaterial()));
	EXPECT_EQ(selected(), modifier.renderSceneGraph().findNodeByUUID(uuid)->volume()->voxel(1, 0, 0));
	brush.setDepth(2);
	modifier.flushPendingBrushChanges();
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	EXPECT_EQ(selected(), modifier.renderSceneGraph().findNodeByUUID(uuid)->volume()->voxel(2, 0, 0));
	modifier.cancelDraft();
	modifier.flushPendingBrushChanges();
	EXPECT_FALSE(modifier.hasDraft());
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
}

TEST_F(BrushOperationTest, testCancelDoesNotReopenAdjustableOperationsOnTheNextFrame) {
	prepare();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("extrude");
	ExtrudeParameters parameters;
	parameters.face = voxel::FaceNames::PositiveX;
	parameters.depth = 1;
	ASSERT_FALSE(modifier.updateParameters<ExtrudeBrush>("extrude", parameters).failed());
	ASSERT_TRUE(modifier.hasDraft());
	modifier.cancelDraft();
	modifier.flushPendingBrushChanges();
	EXPECT_FALSE(modifier.hasDraft());
	modifier.setTool("sculpt");
	ASSERT_FALSE(modifier.parametersChanged().failed());
	ASSERT_TRUE(modifier.hasDraft());
	modifier.cancelDraft();
	modifier.flushPendingBrushChanges();
	EXPECT_FALSE(modifier.hasDraft());
}

TEST_F(BrushOperationTest, testSculptPlaneUsesAccumulatingStrokeAndGroupedUndo) {
	scenegraph::SceneGraphNode &node = prepare();
	voxel::RawVolume *volume = node.volume();
	const core::UUID uuid = node.uuid();
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			volume->setVoxel(x, 0, z, selected());
		}
	}
	_sceneMgr->modified(uuid, voxel::Region(-1, 0, -1, 1, 0, 1));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setTool("sculpt");
	SculptBrush &brush = modifier.sculptBrush();
	brush.setSculptMode(SculptMode::ExtendPlane);
	brush.setBrushRadius(1);
	modifier.flushPendingBrushChanges();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 2));
	EXPECT_FALSE(modifier.isAdjustable());
	modifier.setCursorPosition(glm::ivec3(2, 0, 0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());
	EXPECT_FALSE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	modifier.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.updateEdit());
	modifier.finishEdit(BrushEditFinishReason::Release);
	EXPECT_FALSE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(volume->voxel(4, 0, 0).getMaterial()));
	ASSERT_TRUE(_sceneMgr->undo());
	volume = _sceneMgr->sceneGraphNodeByUUID(uuid)->volume();
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume->voxel(4, 0, 0).getMaterial()));
}

} // namespace voxedit
