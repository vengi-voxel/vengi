/**
 * @file
 */

#include "Modifier.h"
#include "app/App.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "math/Axis.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

Modifier::Modifier(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer)
	: _actionExecuteButton(sceneMgr),
	  _deleteExecuteButton(sceneMgr, ModifierType::Erase), _modifierRenderer(modifierRenderer), _sceneMgr(sceneMgr) {
	registerBuiltinBrushes(_registry, sceneMgr);
	// Note: LUABrush instances are added dynamically during init() via discoverBrushScripts()
}

void Modifier::construct() {
	command::Command::registerActionButton("actionexecute", _actionExecuteButton, "Execute the modifier action");
	command::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton,
										   "Execute the modifier action in delete mode");

	command::Command::registerCommand("actioncolorpicker")
		.setHandler([&](const command::CommandArgs &args) {
			setBrushType(BrushType::None);
			setModifierType(ModifierType::ColorPicker);
		}).setHelp(_("Change the modifier type to 'color picker'"));

	command::Command::registerCommand("actionerase")
		.setHandler([&](const command::CommandArgs &args) {
			setModifierType(ModifierType::Erase);
		}).setHelp(_("Change the modifier type to 'erase'"));

	command::Command::registerCommand("actionplace")
		.setHandler([&](const command::CommandArgs &args) {
			setModifierType(ModifierType::Place);
		}).setHelp(_("Change the modifier type to 'place'"));

	command::Command::registerCommand("actionoverride")
		.setHandler([&](const command::CommandArgs &args) {
			setModifierType(ModifierType::Override);
		}).setHelp(_("Change the modifier type to 'override'"));

	for (const Brush *b : _registry.tools()) {
		const BrushType type = b->type();
		const core::String hlp = core::String::format(_("Change the brush type to '%s'"), b->name().c_str());
		command::Command::registerCommand("brush" + b->name().toLower())
			.setHandler([&, type](const command::CommandArgs &args) {
				setBrushType(type);
			}).setHelp(hlp);
	}
	command::Command::registerCommand("brushnone")
		.setHandler([&](const command::CommandArgs &args) {
			setBrushType(BrushType::None);
		}).setHelp(_("Change the brush type to 'none'"));

	command::Command::registerCommand("brushscript")
		.setHandler([&](const command::CommandArgs &args) {
			setBrushType(BrushType::Script);
		}).setHelp(_("Change the brush type to 'script'"));

	command::Command::registerCommand("brushscriptrescan")
		.setHandler([&](const command::CommandArgs &args) {
			_scriptManager.reloadBrushScripts();
		}).setHelp(_("Re-scan the brushes directory for new or changed Lua brush scripts"));

	command::Command::registerCommand("lock")
		.addArg({"axis", command::ArgType::String, false, "", "Axis to lock: x|y|z"})
		.setHandler([&](const command::CommandArgs &args) {
			const math::Axis axis = math::toAxis(args.str("axis"));
			const bool unlock = (_brushContext.lockedAxis & axis) == axis;
			setLockedAxis(axis, unlock);
		})
		.setHelp(_("Toggle locked mode for the given axis at the current cursor position"))
		.setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

	command::Command::registerCommand("lockx")
		.setHandler([&](const command::CommandArgs &args) {
			const math::Axis axis = math::Axis::X;
			const bool unlock = (_brushContext.lockedAxis & axis) == axis;
			setLockedAxis(axis, unlock);
		}).setHelp(_("Toggle locked mode for the x axis at the current cursor position"));

	command::Command::registerCommand("locky")
		.setHandler([&](const command::CommandArgs &args) {
			const math::Axis axis = math::Axis::Y;
			const bool unlock = (_brushContext.lockedAxis & axis) == axis;
			setLockedAxis(axis, unlock);
		}).setHelp(_("Toggle locked mode for the y axis at the current cursor position"));

	command::Command::registerCommand("lockz")
		.setHandler([&](const command::CommandArgs &args) {
			const math::Axis axis = math::Axis::Z;
			const bool unlock = (_brushContext.lockedAxis & axis) == axis;
			setLockedAxis(axis, unlock);
		}).setHelp(_("Toggle locked mode for the z axis at the current cursor position"));

	const core::VarDef voxEditAutoSelect(cfg::VoxEditAutoSelect, false, N_("Auto select added"), N_("Automatically select newly placed voxels"));
	_autoSelect = core::Var::registerVar(voxEditAutoSelect);

	_previewManager.construct();
	_scriptManager.construct();

	for (Brush *b : _registry.tools()) {
		b->construct();
	}
}

void Modifier::setLockedAxis(math::Axis axis, bool unlock) {
	const math::Axis lockedAxis = unlock ? _brushContext.lockedAxis & ~axis : _brushContext.lockedAxis | axis;
	if (_brushContext.lockedAxis != lockedAxis) {
		finishEdit(BrushEditFinishReason::SettingsChange);
		_brushContext.lockedAxis = lockedAxis;
	}
}

bool Modifier::init() {
	for (Brush *b : _registry.tools()) {
		if (!b->init()) {
			Log::error("Failed to initialize the %s brush", b->name().c_str());
		}
	}
	_scriptManager.setRegistry(_registry);
	_scriptManager.setBeforeScriptChange([this]() {
		++_operationEpoch;
		_handleViewport = -1;
		setBrushGizmoActive(false);
		finishEdit(BrushEditFinishReason::SettingsChange);
		cancelDraft();
	});
	_scriptManager.setSelectBrush(&selectBrush());
	if (!_scriptManager.init()) {
		Log::error("Failed to initialize script manager");
	}
	if (!_previewManager.init(_modifierRenderer)) {
		Log::error("Failed to initialize preview manager");
	}
	if (!_modifierRenderer->init()) {
		Log::error("Failed to initialize modifier renderer");
		return false;
	}
	return true;
}

void Modifier::shutdown() {
	resetPreview();
	reset();
	_scriptManager.shutdown();
	for (Brush *b : _registry.tools()) {
		b->shutdown();
	}
	_previewManager.shutdown();
	_modifierRenderer->shutdown();
}

void Modifier::setHighlightRegion(const voxel::Region &region, uint64_t renderRegionMillis) {
	_modifierRenderer->setHighlightRegion(region, renderRegionMillis);
}

void Modifier::update(double nowSeconds, const video::Camera *camera) {
	_nowSeconds = nowSeconds;
	_brushContext.fixedOrthoSideView = _sliceMode || (camera != nullptr && camera->isOrthoAligned());
	if (Brush *brush = currentBrush()) {
		if (brush->wantsContinuousExecution() && (_actionExecuteButton.pressed() || _deleteExecuteButton.pressed()) &&
			nowSeconds >= _nextContinuousExecution) {
			if (_actionExecuteButton.pressed()) {
				_actionExecuteButton.execute(true);
			}
			if (_deleteExecuteButton.pressed()) {
				_deleteExecuteButton.execute(true);
			}
			_nextContinuousExecution = nowSeconds + 0.1;
		}
	}
	if (Brush *brush = currentBrush()) {
		brush->update(_brushContext, nowSeconds);
	}
}

void Modifier::onSceneChange() {
	++_operationEpoch;
	_handleViewport = -1;
	setBrushGizmoActive(false);
	cancelDraft();
	if (_editSession.isActive()) {
		// Identity of captured volumes is gone; do not write stale runtime onto replacements.
		finishEdit(BrushEditFinishReason::ExternalInvalidate);
	}
	_sliceVolume = nullptr;
	for (Brush *b : _registry.tools()) {
		b->onSceneChange();
	}
}

void Modifier::reset() {
	cancelDraft();
	if (_editSession.isActive()) {
		finishEdit(BrushEditFinishReason::Interrupt);
	}
	_sliceRegion = voxel::Region::InvalidRegion;
	_sliceVolume = nullptr;
	_brushContext.gridResolution = glm::ivec3(1);
	_brushContext.cursorPosition = glm::ivec3(0);
	_brushContext.cursorFace = voxel::FaceNames::Max;
	_brushContext.brushGizmoActive = false;

	_brushContext.modifierType = ModifierType::Place;
	for (Brush *b : _registry.tools()) {
		b->reset();
	}
	setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0));
	setBrushType(BrushType::Shape);
	setModifierType(ModifierType::Place);
}

bool Modifier::beginBrush() {
	if (Brush *brush = currentBrush()) {
		return brush->beginBrush(_brushContext);
	}
	return false;
}

bool Modifier::usesEditSession() const {
	return !_brushGizmoActive && currentBrush() && !isAdjustable();
}

bool Modifier::isStrokeEdit() const {
	return currentBrush() && currentBrush()->interpolatedStroke();
}

bool Modifier::isEditActive() const {
	return _editSession.isActive();
}

void Modifier::restoreEditOverride() {
	if (!_editSession.hasOverride()) {
		return;
	}
	const ModifierType persistent = _editSession.persistentOperation();
	_brushContext.modifierType = currentBrush() ? currentBrush()->modifierType(persistent) : persistent;
}

void Modifier::captureEditTargets() {
	auto addNode = [&](scenegraph::SceneGraphNode &node) {
		if (!node.visible()) {
			return;
		}
		if (_sceneMgr->volume(node.uuid()) == nullptr) {
			return;
		}
		// Capture the model, never the reusable slice scratch volume.
		voxel::RawVolume *volume = node.volume();
		if (volume == nullptr) {
			return;
		}
		_editSession.addTarget(node.uuid(), volume);
		if (auto *target = _editSession.findTarget(node.uuid())) {
			target->evaluator = _registry.evaluation(currentBrush());
			if (target->evaluator) {
				target->evaluator->seedRuntime((uint32_t)_editSession.id());
			}
		}
	};

	if (_editSession.sliceAction()) {
		if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode())) {
			if (!node->locked()) {
				addNode(*node);
			}
		}
		return;
	}
	_sceneMgr->sceneGraph().foreachGroup(addNode);
}

BrushSample Modifier::currentEditSample() const {
	BrushSample sample;
	sample.position = _brushContext.cursorPosition;
	sample.face = _brushContext.cursorFace;
	sample.operation = _editSession.isActive() ? _editSession.resolvedOperation() : _brushContext.modifierType;
	sample.lockedAxis = _brushContext.lockedAxis;
	sample.gridResolution = _brushContext.gridResolution;
	sample.cursorVoxel = _brushContext.cursorVoxel;
	sample.hitCursorVoxel = _brushContext.hitCursorVoxel;
	sample.voxelAtCursor = _brushContext.voxelAtCursor;
	sample.fixedOrthoSideView = _brushContext.fixedOrthoSideView;
	sample.normalPaint = _brushContext.normalPaint;
	sample.normalIndex = _brushContext.normalIndex;
	return sample;
}

BrushApplyResult Modifier::evaluateSample(const BrushSample &sample, BrushApplyTarget &target, bool preview) {
	BrushApplyResult result;
	Brush *brush = currentBrush();
	if (brush == nullptr || _locked || _sceneMgr == nullptr || (!preview && aborted())) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	BrushApplyOptions options;
	options.preview = preview;
	options.applySelectionPolicy = !preview;
	options.autoSelect = autoSelect();
	const BrushEditTarget *active = preview ? _editSession.findTarget(_sceneMgr->activeNodeUUID()) : nullptr;
	auto evaluation = _registry.evaluation(brush, active ? active->evaluator.get() : nullptr);
	if (evaluation) {
		evaluation->syncInput(*brush);
		if (preview && !active) {
			evaluation->seedRuntime((uint32_t)(_editSession.id() + 1));
		}
	}
	if (!evaluation) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	return applyBrushSample(*evaluation.get(), _sceneMgr->sceneGraph(), sample, target, _brushContext, options);
}

void Modifier::notifySessionSample(const BrushEditTarget &target, const BrushApplyResult &result) {
	if (!result.dirtyRegion.isValid() || _sceneMgr == nullptr) {
		return;
	}
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(target.nodeUUID);
	if (node == nullptr || node->volume() != target.volume) {
		return;
	}
	const SceneModifiedFlags flags = SceneModifiedFlags::NoUndo & ~SceneModifiedFlags::ResetTrace;
	_sceneMgr->modified(target.nodeUUID, result.dirtyRegion, flags);
}

BrushApplyResult Modifier::applySessionSample(BrushEditTarget &target, const glm::ivec3 &samplePos) {
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(target.nodeUUID);
	if (node == nullptr || !node->visible() || node->volume() != target.volume) {
		BrushApplyResult result;
		result.status = BrushApplyStatus::InvalidTarget;
		return result;
	}
	voxel::RawVolume *writeVolume = workingVolume(*node);
	if (writeVolume == nullptr) {
		BrushApplyResult result;
		result.status = BrushApplyStatus::InvalidTarget;
		return result;
	}

	BrushSample sample = currentEditSample();
	sample.position = samplePos;

	BrushApplyTarget applyTarget;
	applyTarget.node = node;
	applyTarget.volume = target.volume;
	applyTarget.writeVolume = writeVolume;
	applyTarget.selectionRegion = selectBrush().box3D().selectionRegion();
	applyTarget.sliceRegion = _sliceRegion;

	BrushApplyResult result;
	if (!target.evaluator) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	target.evaluator->syncInput(*currentBrush());
	BrushApplyOptions options;
	options.applySelectionPolicy = true;
	options.autoSelect = autoSelect();
	result =
		applyBrushSample(*target.evaluator.get(), _sceneMgr->sceneGraph(), sample, applyTarget, _brushContext, options);
	if (result.status == BrushApplyStatus::InvalidTarget) {
		return result;
	}
	if (result.dirtyRegion.isValid()) {
		if (writeVolume != node->volume()) {
			node->volume()->copyInto(*writeVolume);
		}
		notifySessionSample(target, result);
	}
	target.paletteChanged |= result.paletteChanged;
	target.normalPaletteChanged |= result.normalPaletteChanged;
	return result;
}

bool Modifier::applySessionTargets() {
	_lastApplyResult = {};
	if (!_editSession.isActive()) {
		return false;
	}
	const glm::ivec3 position = _brushContext.cursorPosition;
	bool anyApplied = false;
	for (BrushEditTarget &target : _editSession.targets()) {
		const BrushApplyResult result = applySessionSample(target, position);
		_editSession.accumulate(target, result.dirtyRegion);
		anyApplied |= result.changed();
		_lastApplyResult.accumulate(result);
		if (result.failed()) {
			return false;
		}
	}
	return anyApplied;
}

bool Modifier::beginEdit(ModifierType overrideType, bool sliceAction) {
	if (_registry.activeDraft()) {
		commit();
	}
	_lastApplyResult = {};
	if (_editSession.isActive()) {
		finishEdit(BrushEditFinishReason::Interrupt);
	}
	if (_locked || !usesEditSession()) {
		return false;
	}

	const ModifierType persistent = _brushContext.modifierType;
	const bool hasOverride = overrideType != ModifierType::None;
	if (hasOverride) {
		setModifierType(overrideType);
		if (!sliceAction) {
			_sceneMgr->trace(false, true);
		}
	}

	if (!beginBrush()) {
		if (hasOverride) {
			_brushContext.modifierType = currentBrush() ? currentBrush()->modifierType(persistent) : persistent;
		}
		return false;
	}

	if (aborted()) {
	if (Brush *brush = currentBrush()) {
			brush->endBrush(_brushContext);
			brush->resetRuntime();
		}
		if (hasOverride) {
			_brushContext.modifierType = currentBrush() ? currentBrush()->modifierType(persistent) : persistent;
		}
		return false;
	}

	++_operationEpoch;
	_editSettingsRevision = currentBrush()->settingsRevision();
	_editSession.begin(_brushType, _brushContext.modifierType, persistent, hasOverride, sliceAction);
	captureEditTargets();
	if (_editSession.targets().empty()) {
		finishEdit(BrushEditFinishReason::Interrupt);
		return false;
	}
	return true;
}

bool Modifier::updateEdit() {
	_lastApplyResult = {};
	if (!_editSession.isActive()) {
		return false;
	}
	if (currentBrush() && currentBrush()->settingsRevision() != _editSettingsRevision) {
		finishEdit(BrushEditFinishReason::SettingsChange);
		return false;
	}
	if (_locked || aborted()) {
		return false;
	}
	Brush *brush = currentBrush();
	if (brush == nullptr) {
		return false;
	}

	if (!isStrokeEdit()) {
		return brush->wantsContinuousExecution() ? applySessionTargets() : true;
	}

	const bool previousSliceMode = _sliceMode;
	if (_editSession.sliceAction()) {
		setSliceMode(true);
	}

	core::DynamicArray<glm::ivec3> samples;
	const glm::ivec3 end = brush->currentCursorPosition(_brushContext);
	_editSession.stroke().visit(end, [&](const glm::ivec3 &position) {
		samples.push_back(position);
	});

	bool anyApplied = false;
	for (BrushEditTarget &target : _editSession.targets()) {
		for (const glm::ivec3 &sample : samples) {
			const BrushApplyResult result = applySessionSample(target, sample);
			_editSession.accumulate(target, result.dirtyRegion);
			anyApplied |= result.changed();
			_lastApplyResult.accumulate(result);
			if (result.failed()) {
				setSliceMode(previousSliceMode);
				finishEdit(BrushEditFinishReason::Interrupt);
				return false;
			}
		}
	}

	setSliceMode(previousSliceMode);
	_lastApplyResult.status = anyApplied ? BrushApplyStatus::Changed : BrushApplyStatus::Unchanged;
	return anyApplied;
}

bool Modifier::finishEdit(BrushEditFinishReason reason) {
	if (!_editSession.isActive()) {
		return true;
	}
	if (currentBrush() && currentBrush()->settingsRevision() != _editSettingsRevision) {
		reason = BrushEditFinishReason::SettingsChange;
	}
	if (reason == BrushEditFinishReason::Release) {
		if (isStrokeEdit()) {
			if (Brush *brush = currentBrush()) {
				// Finish the path without adding an extra stationary paint dab.
				if (!_editSession.stroke().sampled(brush->currentCursorPosition(_brushContext))) {
					updateEdit();
				}
			}
		} else if (_editSession.awaitingSecondAction()) {
			applySessionTargets();
		} else if (needsAdditionalAction()) {
			executeAdditionalAction();
			_editSession.setAwaitingSecondAction(true);
			return true;
		} else {
			applySessionTargets();
		}
	}

	restoreEditOverride();

	if (Brush *brush = currentBrush()) {
		brush->endBrush(_brushContext);
		brush->resetRuntime();
	}

	const bool previousSliceMode = _sliceMode;
	if (_editSession.sliceAction()) {
		setSliceMode(true);
	}

	core::DynamicArray<const BrushEditTarget *> changedTargets;
	for (const BrushEditTarget &target : _editSession.targets()) {
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(target.nodeUUID);
		if ((target.dirtyRegion.isValid() || target.paletteChanged || target.normalPaletteChanged) && node != nullptr &&
			node->volume() == target.volume) {
			changedTargets.push_back(&target);
		}
	}
	// Opening even an empty group truncates redo, so validate targets first.
	if (!changedTargets.empty()) {
		memento::ScopedMementoGroup group(_sceneMgr->mementoHandler(), "edit");
		for (const BrushEditTarget *target : changedTargets) {
			const auto &node = *_sceneMgr->sceneGraphNodeByUUID(target->nodeUUID);
			if (target->paletteChanged) {
				_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
			}
			if (target->normalPaletteChanged) {
				_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), node);
			}
			if (target->dirtyRegion.isValid()) {
				// Keep the live pick valid after voxels change under the cursor (e.g. plane extrude).
				_sceneMgr->modified(target->nodeUUID, target->dirtyRegion,
									SceneModifiedFlags::MarkUndo | SceneModifiedFlags::ResetTrace);
			}
		}
	}

	setSliceMode(previousSliceMode);
	const bool refreshTrace = _editSession.hasOverride() && !_editSession.sliceAction();
	_editSession.end();
	++_operationEpoch;
	_handleViewport = -1;
	setBrushGizmoActive(false);
	if (refreshTrace) {
		_sceneMgr->trace(false, true);
	}
	return true;
}

void Modifier::executeAdditionalAction() {
	if (isMode(ModifierType::ColorPicker)) {
		return;
	}
	if (Brush *brush = currentBrush()) {
		brush->step(_brushContext);
	}
}

void Modifier::setReferencePosition(const glm::ivec3 &pos) {
	_brushContext.referencePos = pos;
}

bool Modifier::needsAdditionalAction() {
	if (_sliceRegion.isValid()) {
		return false;
	}
	if (const Brush *brush = currentBrush()) {
		return brush->needsAdditionalAction(_brushContext);
	}
	return false;
}

glm::ivec3 Modifier::currentCursorPosition() {
	if (Brush *brush = currentBrush()) {
		return brush->currentCursorPosition(_brushContext);
	}
	return _brushContext.cursorPosition;
}

voxel::Region Modifier::calcBrushRegion() {
	Brush *brush = currentBrush();
	if (brush) {
		return brush->calcRegion(_brushContext);
	}
	return voxel::Region::InvalidRegion;
}

voxel::RawVolumeWrapper Modifier::createRawVolumeWrapper(voxel::RawVolume *volume) const {
	return voxel::RawVolumeWrapper(volume);
}

void Modifier::setHitCursorVoxel(const voxel::Voxel &voxel) {
	_brushContext.hitCursorVoxel = voxel;
	_brushContext.hitCursorVoxel.setNormal(voxel.getNormal());
}

void Modifier::setVoxelAtCursor(const voxel::Voxel &voxel) {
	_brushContext.voxelAtCursor = voxel;
	_brushContext.voxelAtCursor.setNormal(voxel.getNormal());
}

void Modifier::lock() {
	_locked = true;
}

void Modifier::unlock() {
	_locked = false;
}

void Modifier::setSliceRegion(const voxel::Region &region) {
	if (_sliceRegion == region) {
		return;
	}
	if (_editSession.isActive()) {
		// The 3D viewport clears the slice region every frame. That is not a user
		// slice change. Keep a held 2D edit on its captured plane, and do not abort
		// a 3D stroke just because a viewport reset the slice state.
		if (!region.isValid()) {
			if (_editSession.sliceAction()) {
				return;
			}
		} else {
			finishEdit(BrushEditFinishReason::SliceChange);
		}
	}
	cancelDraft();
	if (Brush *brush = currentBrush()) {
		brush->onSceneChange();
	}
	_sliceRegion = region;
	_sliceVolume = nullptr;
}

voxel::RawVolume *Modifier::workingVolume(scenegraph::SceneGraphNode &node) {
	if (!_sliceRegion.isValid()) {
		return node.volume();
	}
	voxel::Region region = _sliceRegion;
	if (!region.cropTo(node.region())) {
		return nullptr;
	}
	if (!_sliceVolume || _sliceVolume->region() != region || _sliceNodeUUID != node.uuid()) {
		_sliceVolume = core::make_shared<voxel::RawVolume>(*node.volume(), region);
		_sliceNodeUUID = node.uuid();
	} else {
		_sliceVolume->copyInto(*node.volume());
	}
	return _sliceVolume.get();
}

bool Modifier::execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
					   const ModifiedRegionCallback &callback) {
	if (_locked) {
		return false;
	}
	if (aborted()) {
		return false;
	}

	voxel::RawVolume *volume = workingVolume(node);
	if (volume == nullptr) {
		Log::debug("No volume given - can't perform action");
		return false;
	}

	if (isMode(ModifierType::ColorPicker)) {
		setCursorVoxel(hitCursorVoxel());
		return true;
	}

	preExecuteBrush(volume);
	const voxel::Voxel cursorVoxel = _brushContext.cursorVoxel;
	if (_brushContext.modifierType == ModifierType::NormalPaint) {
		_brushContext.cursorVoxel.setNormal(_brushContext.normalIndex + NORMAL_PALETTE_OFFSET);
	}
	const bool success = executeBrush(sceneGraph, node, _brushContext.modifierType, _brushContext.cursorVoxel, callback);
	_brushContext.cursorVoxel = cursorVoxel; // set back the original voxel
	return success;
}

void Modifier::preExecuteBrush(const voxel::RawVolume *volume) {
	core_trace_scoped(ModifierPrepareBrush);
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	_brushContext.targetVolumeRegion = volume->region();
	_brushContext.prevCursorPosition = _brushContext.cursorPosition;
	if (brush->clampToVolume()) {
		const voxel::Region brushRegion = brush->calcRegion(_brushContext);
		_brushContext.cursorPosition =
			clampBrushCursor(_brushContext.targetVolumeRegion, brushRegion, _brushContext.prevCursorPosition);
	}
	if (_sliceRegion.isValid()) {
		_brushContext.cursorPosition = glm::clamp(_brushContext.cursorPosition, _sliceRegion.getLowerCorner(), _sliceRegion.getUpperCorner());
	}
	brush->preExecute(_brushContext, volume);
}

bool Modifier::executeBrush(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							ModifierType modifierType, const voxel::Voxel &voxel,
							const ModifiedRegionCallback &callback, bool preview) {
	BrushSample sample = currentEditSample();
	sample.operation = modifierType;
	sample.cursorVoxel = voxel;
	BrushApplyTarget target;
	target.node = &node;
	target.volume = node.volume();
	target.writeVolume = workingVolume(node);
	target.selectionRegion = selectBrush().box3D().selectionRegion();
	target.sliceRegion = _sliceRegion;
	const auto result = evaluateSample(sample, target, preview);
	if (!preview && result.dirtyRegion.isValid() && target.writeVolume != target.volume) {
		target.volume->copyInto(*target.writeVolume);
	}
	if (callback && result.dirtyRegion.isValid()) {
						callback(result.dirtyRegion, modifierType, SceneModifiedFlags::All);
					}
	return !result.failed();
}

Brush *Modifier::currentBrush() {
	if (Brush *tool = _registry.find(_toolId)) {
		return tool;
	}
	return _brushType == BrushType::Script ? _scriptManager.activeLuaBrush() : nullptr;
}

const Brush *Modifier::currentBrush() const {
	if (Brush *tool = _registry.find(_toolId)) {
		return tool;
	}
	return _brushType == BrushType::Script ? _scriptManager.activeLuaBrush() : nullptr;
}

AABBBrush *Modifier::currentAABBBrush() {
	Brush *brush = currentBrush();
	return brush ? brush->aabb() : nullptr;
}

const AABBBrush *Modifier::currentAABBBrush() const {
	const Brush *brush = currentBrush();
	return brush ? brush->aabb() : nullptr;
}

bool Modifier::setTool(const core::String &id) {
	Brush *tool = _registry.find(id);
	if (!tool) {
		return false;
	}
	if (_toolId != id) {
		commit();
		_operationError = nullptr;
		++_operationEpoch;
		_handleViewport = -1;
		setBrushGizmoActive(false);
		_toolId = id;
		_brushType = tool->type();
		_brushContext.modifierType = tool->modifierType(_brushContext.modifierType);
		if ((tool->volumeModifier(tool->modifierType()) & ModifierType::NormalPaint) == ModifierType::None) {
			_brushContext.normalIndex = NO_NORMAL;
			_brushContext.cursorVoxel.setNormal(NO_NORMAL);
	}
		const auto &scripts = _scriptManager.luaBrushes();
		for (size_t i = 0; i < scripts.size(); ++i) {
			if (scripts[i] == tool) {
				_scriptManager.setActiveLuaBrushIndex((int)i);
				break;
	}
		}
		tool->onActivated();
		resetPreview();
	}
	return true;
}

void Modifier::endBrush() {
	if (Brush* brush = currentBrush()) {
		brush->endBrush(_brushContext);
	}
}

void Modifier::abort() {
	if (isAdjustable()) {
		cancelDraft();
		return;
	}
	if (_editSession.isActive()) {
		finishEdit(BrushEditFinishReason::Interrupt);
		return;
	}
	if (Brush *brush = currentBrush()) {
		brush->abort(_brushContext);
		brush->resetRuntime();
	}
}

void Modifier::commit() {
	if (_registry.activeDraft()) {
		if (applyDraft().failed()) {
			cancelDraft();
		}
		return;
	}
	finishEdit(BrushEditFinishReason::ToolChange);
	if (Brush *brush = currentBrush()) {
		brush->resetRuntime();
	}
}

void Modifier::brushApply() {
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	commit();
	brush->onActivated();
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return (currentBrush() && (currentBrush()->volumeModifier(_brushContext.modifierType) &
							   ModifierType::NormalPaint) != ModifierType::None) || isMode(ModifierType::ExistingVoxelMask);
}

BrushType Modifier::setBrushType(BrushType type) {
	const Brush *registered = type == BrushType::Script ? _scriptManager.activeLuaBrush() : _registry.find(type);
	if (_brushType == type && currentBrush() == registered) {
		return _brushType;
	}
	if (_editSession.isActive()) {
		finishEdit(BrushEditFinishReason::ToolChange);
	}

	// Auto-commit pending changes from the current brush before switching.
	// Must happen before changing _brushType so currentBrush() returns the old brush.
	commit();
	_operationError = nullptr;
	resetPreview();

	if (!registered ||
		(registered->volumeModifier(registered->modifierType()) & ModifierType::NormalPaint) == ModifierType::None) {
		_brushContext.normalIndex = NO_NORMAL;
		_brushContext.cursorVoxel.setNormal(NO_NORMAL);
	}
	++_operationEpoch;
	_handleViewport = -1;
	setBrushGizmoActive(false);
	_brushType = type;
	if (registered) {
		_toolId = _registry.id(*registered);
	} else {
		_toolId.clear();
	}
	Brush *newBrush = currentBrush();
	if (newBrush) {
		setModifierType(newBrush->modifierType(_brushContext.modifierType));
		newBrush->onActivated();
	}
	return _brushType;
}

void Modifier::setGridResolution(const glm::ivec3 &gridSize) {
	const glm::ivec3 resolution = glm::max(glm::ivec3(1), gridSize);
	if (_brushContext.gridResolution != resolution) {
		finishEdit(BrushEditFinishReason::SettingsChange);
		_brushContext.gridResolution = resolution;
	}
}

void Modifier::setGridResolution(int gridSize) {
	setGridResolution(glm::ivec3(core_max(1, gridSize)));
}

ModifierType Modifier::setModifierType(ModifierType type) {
	const ModifierType resolved = currentBrush() ? currentBrush()->modifierType(type) : type;
	if (_brushContext.modifierType == resolved) {
		return resolved;
	}
	if (_editSession.isActive()) {
		finishEdit(BrushEditFinishReason::OperationChange);
	}
	const ModifierType oldType = _brushContext.modifierType;
	if (currentBrush()) {
		_brushContext.modifierType = currentBrush()->modifierType(type);
		if (oldType != _brushContext.modifierType) {
			currentBrush()->markDirty();
		}
	} else {
		_brushContext.modifierType = type;
	}
	return _brushContext.modifierType;
}

void Modifier::resetPreview() {
	_previewManager.resetPreview();
}

void Modifier::flushPendingBrushChanges() {
	if (_editSession.isActive() && currentBrush() && currentBrush()->settingsRevision() != _editSettingsRevision) {
		finishEdit(BrushEditFinishReason::SettingsChange);
	}
	if (_registry.activeDraft() && !isAdjustable()) {
		commit();
	}
	if (isAdjustable()) {
		if (currentBrush()->dirty()) {
			parametersChanged();
			currentBrush()->markClean();
		}
		return;
	}
}

ModifierType Modifier::checkModifierType() {
	return currentBrush() ? currentBrush()->modifierType(ModifierType::Mask) : ModifierType::ColorPicker;
}

void Modifier::render(voxelrender::RenderContext &renderContext, const video::Camera &camera, palette::Palette &activePalette, const glm::mat4 &model) {
	if (_locked) {
		return;
	}

	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	Brush *brush = currentBrush();

	// Build the context for the renderer
	ModifierRendererContext ctx;
	ctx.cursorVoxel = _brushContext.cursorVoxel;
	ctx.voxelAtCursor = _brushContext.voxelAtCursor;
	ctx.cursorFace = _brushContext.cursorFace;
	ctx.cursorPosition = _brushContext.cursorPosition;
	ctx.gridResolution = _brushContext.gridResolution;
	ctx.referencePosition = referencePosition();
	ctx.palette = &activePalette;
	ctx.brushActive = false;
	if (brush) {
		const ModifierType operation = brush->volumeModifier(_brushContext.modifierType);
		ctx.previewErase = (operation & ModifierType::Erase) != ModifierType::None &&
						   (operation & ModifierType::NormalPaint) == ModifierType::None &&
						   brush->previewStyle() != BrushPreviewStyle::Selection;
	}

	// Mirror plane info
	if (brush) {
		ctx.mirrorAxis = brush->mirrorAxis();
		ctx.mirrorPos = brush->mirrorPos();
	}
	if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNodeByUUID(sceneGraph.activeNodeUUID())) {
		ctx.activeRegion = node->region();
		if (brush &&
			(brush->volumeModifier(_brushContext.modifierType) & ModifierType::NormalPaint) != ModifierType::None) {
			ctx.normalPalette = &node->normalPalette();
		}
	}

	if (!_sliceRegion.isValid() && !isMode(ModifierType::ColorPicker)) {
		ctx.brushActive = brush && brush->active() && !isAdjustable();
	}

	if (!_sliceRegion.isValid() && brush &&
		(brush->volumeModifier(_brushContext.modifierType) & ModifierType::NormalPaint) != ModifierType::None && !core::getVar(cfg::RenderNormals)->boolVal()) {
		ctx.brushActive = false;
		brush->markDirty();
	}

	if (ctx.brushActive) {
		if (brush->dirty()) {
			// Clear stale preview so the old position does not keep
			// rendering, then regenerate immediately at the new position.
			resetPreview();
			brush->markClean();
			_previewManager.scheduleUpdate(_nowSeconds);
		}
		voxel::RawVolume *activeVolume = _sceneMgr->volume(sceneGraph.activeNodeUUID());
		_previewManager.checkPendingUpdate(_nowSeconds, *this, activePalette, activeVolume, sceneGraph);
		// Copy cached preview state to renderer context
		ctx.previewVolume = _previewManager.previewVolume();
		ctx.previewMirrorVolume = _previewManager.previewMirrorVolume();
		const BrushPreview &preview = _previewManager.brushPreview();
		if (preview.useSimplePreview) {
			ctx.useSimplePreview = true;
			ctx.simplePreviewRegion = preview.simplePreviewRegion;
			ctx.simpleMirrorPreviewRegion = preview.simpleMirrorPreviewRegion;
			ctx.simplePreviewColor = preview.simplePreviewColor;
		}
		if (preview.showOutlinePreview) {
			ctx.showOutlinePreview = true;
			ctx.outlinePreviewPoints = preview.outlinePreviewPoints;
			ctx.outlineMirrorPreviewPoints = preview.outlineMirrorPreviewPoints;
			ctx.outlinePreviewColor = preview.outlinePreviewColor;
		}
		if (ctx.previewVolume || ctx.useSimplePreview || ctx.showOutlinePreview) {
			ctx.palette = _previewManager.previewPalette();
			if (ctx.previewVolume) {
				ctx.normalPalette = _previewManager.previewNormalPalette();
			}
		}
	} else {
		// Clear cached preview state when the brush is not active to prevent stale
		// previews from reappearing as ghosts when the brush becomes active again
		resetPreview();
	}

	// Let the renderer handle buffer updates and rendering
	_modifierRenderer->update(ctx);

	// Render everything
	_modifierRenderer->render(renderContext, camera, model);
}

} // namespace voxedit
