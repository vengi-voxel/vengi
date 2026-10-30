/**
 * @file
 */

#include "Modifier.h"
#include "app/I18NMarkers.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool Modifier::updateBoxRegion(const voxel::Region &region) {
	AABBBrush *brush = currentAABBBrush();
	if (_locked || !brush || !_editSession.isActive()) {
		return false;
	}
	return brush->setRegion(region);
}

BrushApplyResult Modifier::executeRequestFactory(const core::Function<core::SharedPtr<Brush>()> &create,
												 const BrushContext &input, const core::Buffer<core::UUID> &targets) {
	BrushApplyResult result;
	if (_locked) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	// Resolve the interactive transaction before a command writes to its inputs.
	commit();
	core::Buffer<core::UUID> unique;
	for (const auto &uuid : targets) {
		auto *node = _sceneMgr->sceneGraphNodeByUUID(uuid);
		if (!node || !node->volume()) {
			result.status = BrushApplyStatus::InvalidTarget;
			return result;
		}
		bool duplicate = false;
		for (const auto &previous : unique) {
			duplicate |= previous == uuid;
		}
		if (!duplicate) {
			unique.push_back(uuid);
		}
	}
	core::DynamicArray<BrushApplyResult> records;
	BrushSample sample;
	sample.position = input.cursorPosition;
	sample.face = input.cursorFace;
	sample.operation = input.modifierType;
	sample.lockedAxis = input.lockedAxis;
	sample.gridResolution = input.gridResolution;
	sample.cursorVoxel = input.cursorVoxel;
	sample.hitCursorVoxel = input.hitCursorVoxel;
	sample.voxelAtCursor = input.voxelAtCursor;
	sample.fixedOrthoSideView = input.fixedOrthoSideView;
	sample.normalPaint = input.normalPaint;
	sample.normalIndex = input.normalIndex;
	for (const auto &uuid : unique) {
		auto *node = _sceneMgr->sceneGraphNodeByUUID(uuid);
		auto tool = create();
		if (!tool) {
			result.status = BrushApplyStatus::Failed;
			break;
		}
		sample.operation = tool->modifierType(input.modifierType);
		tool->seedRuntime(1);
		BrushApplyTarget target;
		target.node = node;
		target.volume = node->volume();
		BrushApplyOptions options;
		options.applySelectionPolicy = true;
		options.autoSelect = autoSelect();
		const auto applied = applyBrushSample(*tool.get(), _sceneMgr->sceneGraph(), sample, target, input, options);
		records.push_back(applied);
		result.accumulate(applied);
		if (applied.failed()) {
			break;
		}
	}
	if (result.changed()) {
		memento::ScopedMementoGroup group(_sceneMgr->mementoHandler(), "edit");
		for (size_t i = 0; i < records.size(); ++i) {
			const auto &record = records[i];
			const auto &node = *_sceneMgr->sceneGraphNodeByUUID(unique[i]);
			if (record.paletteChanged) {
				_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
			}
			if (record.normalPaletteChanged) {
				_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), node);
			}
			if (record.dirtyRegion.isValid()) {
				_sceneMgr->modified(unique[i], record.dirtyRegion, SceneModifiedFlags::All);
			} else if (record.paletteChanged || record.normalPaletteChanged) {
				_sceneMgr->modified(unique[i], node.region(), SceneModifiedFlags::NoUndo);
			}
		}
	}
	return result;
}

BrushApplyResult Modifier::invalidParameters() {
	_operationError = N_("Invalid operation parameters");
	BrushApplyResult result;
	result.status = BrushApplyStatus::Failed;
	return result;
}

BrushApplyResult Modifier::parametersChanged() {
	BrushApplyResult result;
	BrushDraft *draft = _registry.draft(currentBrush());
	if (_locked || !draft) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	_operationError = nullptr;
	_sceneMgr->waitForPendingExtractions();
	core::Buffer<core::UUID> targets;
	_sceneMgr->sceneGraph().foreachGroup([&](scenegraph::SceneGraphNode &node) {
		if (node.visible() && node.volume() &&
			(!_sliceRegion.isValid() || (node.uuid() == _sceneMgr->activeNodeUUID() && !node.locked()))) {
			targets.push_back(node.uuid());
		}
	});
	BrushApplyOptions options;
	options.applySelectionPolicy = !currentBrush()->managesOwnSelection();
	options.autoSelect =
		autoSelect() && currentBrush()->autoSelectPlacement() && _brushContext.modifierType == ModifierType::Place;
	result = draft->refresh(_sceneMgr->sceneGraph(), targets, _sliceRegion, _brushContext, options,
							[this]() { return applyDraft(); });
	if (result.failed()) {
		_operationError = result.status == BrushApplyStatus::InvalidTarget
							  ? N_("The edit target changed or has no selection")
							  : N_("Invalid operation parameters");
		return result;
	}
	if (const auto *view = draft->view()) {
		for (auto entry : view->nodes()) {
			const auto &node = entry->value;
			if (node.isModelNode()) {
				_sceneMgr->updatePreviewRegion(node.uuid(), node.region());
			}
		}
	}
	currentBrush()->markClean();
	return result;
}

BrushApplyResult Modifier::applyDraft() {
	BrushApplyResult result;
	BrushDraft *draft = _registry.activeDraft();
	if (!draft) {
		return result;
	}
	if (_operationError) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	_sceneMgr->waitForPendingExtractions();
	result = draft->apply(_sceneMgr->sceneGraph());
	if (result.changed()) {
		memento::ScopedMementoGroup group(_sceneMgr->mementoHandler(), "edit");
		for (auto entry : _sceneMgr->sceneGraph().nodes()) {
			const auto &node = entry->value;
			const voxel::Region dirty = draft->dirtyRegion(node.uuid());
			if (draft->paletteChanged(node.uuid())) {
				_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
			}
			if (draft->normalPaletteChanged(node.uuid())) {
				_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), node);
			}
			if (dirty.isValid()) {
				_sceneMgr->modified(node.uuid(), dirty, SceneModifiedFlags::All);
			} else if (draft->paletteChanged(node.uuid()) || draft->normalPaletteChanged(node.uuid())) {
				_sceneMgr->modified(node.uuid(), node.region(), SceneModifiedFlags::NoUndo);
			}
		}
	}
	cancelDraft();
	if (result.failed()) {
		_operationError = N_("The edit target changed");
	}
	return result;
}

void Modifier::cancelDraft() {
	_operationError = nullptr;
	if (_registry.activeDraft()) {
		++_operationEpoch;
		_handleViewport = -1;
		setBrushGizmoActive(false);
	}
	while (BrushDraft *draft = _registry.activeDraft()) {
		_sceneMgr->waitForPendingExtractions();
		_sceneMgr->releasePreview(*draft->view());
		draft->clear();
		if (currentBrush()) {
			currentBrush()->markClean();
		}
	}
}

const scenegraph::SceneGraph &Modifier::renderSceneGraph() {
	if (BrushDraft *draft = _registry.activeDraft()) {
		if (draft->valid(_sceneMgr->sceneGraph())) {
			return *draft->view();
		}
		cancelDraft();
	}
	return _sceneMgr->sceneGraph();
}

void Modifier::brushHandles(core::DynamicArray<BrushGizmoState> &output) {
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	if (isAdjustable()) {
		const auto *node = _sceneMgr->sceneGraphModelNodeByUUID(_sceneMgr->activeNodeUUID());
		if (!node || (_registry.draft(brush)->requiresSelection() && !node->hasSelection())) {
			return;
		}
		BrushContext input = _brushContext;
		input.targetVolumeRegion = node->region();
		brush->preExecute(input, node->volume());
	}
	brush->handles(_brushContext, output);
	for (auto &handle : output) {
		handle.operationId = _operationEpoch;
	}
}

bool Modifier::editBrushHandle(const BrushHandleEvent &event) {
	if (_locked || event.operationId != _operationEpoch) {
		return false;
	}
	if (event.phase == BrushHandlePhase::Begin) {
		if (isBrushActionActive()) {
			return false;
		}
		core::DynamicArray<BrushGizmoState> handles;
		brushHandles(handles);
		bool exists = false;
		for (const auto &handle : handles) {
			exists |= handle.id == event.handleId;
		}
		if (!exists) {
			return false;
		}
		if (_handleViewport != -1 && (_handleViewport != event.viewportId || _capturedHandle != event.handleId)) {
			return false;
		}
		_handleViewport = event.viewportId;
		_capturedHandle = event.handleId;
		setBrushGizmoActive(true);
		return true;
	}
	if (_handleViewport != event.viewportId || _capturedHandle != event.handleId) {
		return false;
	}
	if (event.phase == BrushHandlePhase::End) {
		_handleViewport = -1;
		setBrushGizmoActive(false);
		return true;
	}
	core::DynamicArray<BrushGizmoState> handles;
	brushHandles(handles);
	for (const auto &handle : handles) {
		if (handle.id == event.handleId) {
			const uint32_t operation = event.operation ? event.operation : handle.operations;
			if (operation & ~handle.operations) {
				return false;
			}
			if (!currentBrush()->editHandle(event.handleId, _brushContext, event.matrix, event.delta, operation)) {
				return false;
			}
			return !isAdjustable() || !parametersChanged().failed();
		}
	}
	return false;
}

} // namespace voxedit
