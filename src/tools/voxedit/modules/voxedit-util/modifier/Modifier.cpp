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
#include "io/Filesystem.h"
#include "math/Axis.h"
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
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

Modifier::Modifier(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer)
	: _stampBrush(sceneMgr), _selectBrush(sceneMgr), _textureBrush(sceneMgr), _actionExecuteButton(sceneMgr),
	  _deleteExecuteButton(sceneMgr, ModifierType::Erase), _modifierRenderer(modifierRenderer), _sceneMgr(sceneMgr) {
	_brushes.push_back(&_planeBrush);
	_brushes.push_back(&_shapeBrush);
	_brushes.push_back(&_stampBrush);
	_brushes.push_back(&_lineBrush);
	_brushes.push_back(&_paintBrush);
	_brushes.push_back(&_textBrush);
	_brushes.push_back(&_selectBrush);
	_brushes.push_back(&_textureBrush);
	_brushes.push_back(&_normalBrush);
	_brushes.push_back(&_extrudeBrush);
	_brushes.push_back(&_transformBrush);
	_brushes.push_back(&_sculptBrush);
	_brushes.push_back(&_rulerBrush);
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

	for (const Brush *b : _brushes) {
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

	for (Brush *b : _brushes) {
		b->construct();
	}
}

void Modifier::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_brushContext.lockedAxis &= ~axis;
	} else {
		_brushContext.lockedAxis |= axis;
	}
}

bool Modifier::init() {
	for (Brush *b : _brushes) {
		if (!b->init()) {
			Log::error("Failed to initialize the %s brush", b->name().c_str());
		}
	}
	_scriptManager.setSelectBrush(&_selectBrush);
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
	for (Brush *b : _brushes) {
		b->shutdown();
	}
	_scriptManager.shutdown();
	_previewManager.shutdown();
	_modifierRenderer->shutdown();
}

void Modifier::update(double nowSeconds, const video::Camera *camera) {
	_nowSeconds = nowSeconds;
	_brushContext.fixedOrthoSideView = camera == nullptr ? false : camera->isOrthoAligned();
	if (AABBBrush *aabbBrush = currentAABBBrush()) {
		if (aabbBrush->anySingleMode()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
	} else if (_brushType == BrushType::Stamp) {
		if (_stampBrush.continuousMode()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
	} else if (Brush *b = currentBrush()) {
		if (b->wantsContinuousExecution()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
	}
	if (Brush *brush = currentBrush()) {
		brush->update(_brushContext, nowSeconds);
	}
}

void Modifier::onSceneChange() {
	for (Brush *b : _brushes) {
		b->onSceneChange();
	}
}

void Modifier::reset() {
	_brushContext.gridResolution = 1;
	_brushContext.cursorPosition = glm::ivec3(0);
	_brushContext.cursorFace = voxel::FaceNames::Max;
	_brushContext.brushGizmoActive = false;

	_brushContext.modifierType = ModifierType::Place;
	for (Brush *b : _brushes) {
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

bool Modifier::beginBrushFromPanel() {
	if (Brush *brush = currentBrush()) {
		BrushContext ctx = _brushContext;
		ctx.cursorFace = voxel::FaceNames::Max;
		return brush->beginBrush(ctx);
	}
	return false;
}

void Modifier::executeAdditionalAction() {
	if (isMode(ModifierType::ColorPicker)) {
		return;
	}
	if (AABBBrush *brush = currentAABBBrush()) {
		brush->step(_brushContext);
	}
}

void Modifier::setReferencePosition(const glm::ivec3 &pos) {
	_brushContext.referencePos = pos;
}

bool Modifier::needsAdditionalAction() {
	if (const AABBBrush *brush = currentAABBBrush()) {
		return brush->needsAdditionalAction(_brushContext);
	}
	return false;
}

glm::ivec3 Modifier::currentCursorPosition() {
	if (AABBBrush *brush = currentAABBBrush()) {
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
}

void Modifier::setVoxelAtCursor(const voxel::Voxel &voxel) {
	_brushContext.voxelAtCursor = voxel;
}

void Modifier::lock() {
	_locked = true;
}

void Modifier::unlock() {
	_locked = false;
}

bool Modifier::execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
					   const ModifiedRegionCallback &callback) {
	if (_locked) {
		return false;
	}
	if (aborted()) {
		return false;
	}

	voxel::RawVolume *volume = node.volume();
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
		_brushContext.cursorVoxel.setNormal(_brushContext.normalIndex);
	}
	executeBrush(sceneGraph, node, _brushContext.modifierType, _brushContext.cursorVoxel, callback);
	_brushContext.cursorVoxel = cursorVoxel; // set back the original voxel
	return true;
}

/**
 * change the cursor position if the brush region is outside the volume
 * this allows us to keep all voxels inside the volume boundaries even on the
 * +x, +y and +z sides where the voxels are currently flowing out of the volume
 */
static glm::ivec3 updateCursor(const voxel::Region &region, const voxel::Region &brushRegion,
							   const glm::ivec3 &cursor) {
	if (!brushRegion.isValid()) {
		return cursor;
	}
	if (region.containsRegion(brushRegion)) {
		return cursor;
	}
	glm::ivec3 delta(0);
	if (brushRegion.getUpperX() > region.getUpperX()) {
		delta.x = region.getUpperX() - brushRegion.getUpperX();
	}
	if (brushRegion.getUpperY() > region.getUpperY()) {
		delta.y = region.getUpperY() - brushRegion.getUpperY();
	}
	if (brushRegion.getUpperZ() > region.getUpperZ()) {
		delta.z = region.getUpperZ() - brushRegion.getUpperZ();
	}

	if (brushRegion.getLowerX() < region.getLowerX()) {
		delta.x = glm::abs(region.getLowerX() - brushRegion.getUpperX());
	}
	if (brushRegion.getLowerY() < region.getLowerY()) {
		delta.y = glm::abs(region.getLowerY() - brushRegion.getLowerY());
	}
	if (brushRegion.getLowerZ() < region.getLowerZ()) {
		delta.z = glm::abs(region.getLowerZ() - brushRegion.getLowerZ());
	}

	return cursor + delta;
}

void Modifier::preExecuteBrush(const voxel::RawVolume *volume) {
	core_trace_scoped(ModifierPrepareBrush);
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	_brushContext.targetVolumeRegion = volume->region();
	_brushContext.prevCursorPosition = _brushContext.cursorPosition;
	if (brush->brushClamping()) {
		const voxel::Region brushRegion = brush->calcRegion(_brushContext);
		_brushContext.cursorPosition =
			updateCursor(_brushContext.targetVolumeRegion, brushRegion, _brushContext.prevCursorPosition);
	}
	brush->preExecute(_brushContext, volume);
}

bool Modifier::executeBrush(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							ModifierType modifierType, const voxel::Voxel &voxel,
							const ModifiedRegionCallback &callback) {
	core_trace_scoped(ModifierExecuteBrush);
	Brush *brush = currentBrush();
	if (!brush) {
		return false;
	}
	ModifierVolumeWrapper wrapper(node, modifierType, _selectBrush.box3DSelectionRegion());
	voxel::Voxel prevVoxel = _brushContext.cursorVoxel;
	glm::ivec3 prevCursorPos = _brushContext.cursorPosition;
	if (brush->brushClamping()) {
		const voxel::Region brushRegion = brush->calcRegion(_brushContext);
		_brushContext.cursorPosition = updateCursor(_brushContext.targetVolumeRegion, brushRegion, prevCursorPos);
	}
	_brushContext.cursorVoxel = voxel;
	brush->execute(sceneGraph, wrapper, _brushContext);
	const bool isPlacementBrush = brush->type() != BrushType::Select && brush->type() != BrushType::Paint
		&& brush->type() != BrushType::Normal;
	const bool isPlacementModifier = modifierType == ModifierType::Place;
	if (autoSelect() && isPlacementBrush && isPlacementModifier) {
		wrapper.autoSelectNewVoxels();
	} else if (!brush->managesOwnSelection()) {
		wrapper.growSelectionToNewVoxels();
	}
	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		voxel::logRegion("Dirty region", modifiedRegion);
		if (callback) {
			const SceneModifiedFlags flags = brush->sceneModifiedFlags();
			callback(modifiedRegion, _brushContext.modifierType, flags);
		}
	}
	_brushContext.cursorPosition = prevCursorPos;
	_brushContext.cursorVoxel = prevVoxel;
	return true;
}

Brush *Modifier::currentBrush() {
	if (_brushType == BrushType::Script) {
		return _scriptManager.activeLuaBrush();
	}
	for (Brush *b : _brushes) {
		if (b->type() == _brushType) {
			return b;
		}
	}
	return nullptr;
}

const Brush *Modifier::currentBrush() const {
	if (_brushType == BrushType::Script) {
		return _scriptManager.activeLuaBrush();
	}
	for (Brush *b : _brushes) {
		if (b->type() == _brushType) {
			return b;
		}
	}
	return nullptr;
}

AABBBrush *Modifier::currentAABBBrush() {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	if (_brushType == BrushType::Paint) {
		return &_paintBrush;
	}
	if (_brushType == BrushType::Select) {
		return &_selectBrush;
	}
	if (_brushType == BrushType::Plane) {
		return &_planeBrush;
	}
	if (_brushType == BrushType::Texture) {
		return &_textureBrush;
	}
	if (_brushType == BrushType::Normal) {
		return &_normalBrush;
	}
	if (_brushType == BrushType::Script) {
		return static_cast<AABBBrush *>(_scriptManager.activeLuaBrush());
	}
	return nullptr;
}

const AABBBrush *Modifier::currentAABBBrush() const {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	if (_brushType == BrushType::Paint) {
		return &_paintBrush;
	}
	if (_brushType == BrushType::Select) {
		return &_selectBrush;
	}
	if (_brushType == BrushType::Plane) {
		return &_planeBrush;
	}
	if (_brushType == BrushType::Texture) {
		return &_textureBrush;
	}
	if (_brushType == BrushType::Normal) {
		return &_normalBrush;
	}
	if (_brushType == BrushType::Script) {
		return static_cast<const AABBBrush *>(_scriptManager.activeLuaBrush());
	}
	return nullptr;
}

void Modifier::endBrush() {
	if (Brush* brush = currentBrush()) {
		brush->endBrush(_brushContext);
	}
}

void Modifier::abort() {
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	if (brush->hasPendingChanges()) {
		_sceneMgr->nodeForeachGroup([&](int nodeId) {
			scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
			if (node == nullptr || !node->visible()) {
				return;
			}
			voxel::RawVolume *volume = node->volume();
			if (volume == nullptr) {
				return;
			}
			const voxel::Region dirtyRegion = brush->revertChanges(volume);
			if (dirtyRegion.isValid()) {
				_sceneMgr->modified(nodeId, dirtyRegion, SceneModifiedFlags::NoUndo);
			}
		});
		brush->reset();
		brush->onActivated();
		return;
	}
	brush->abort(_brushContext);
}

void Modifier::commit() {
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	if (!brush->onDeactivated()) {
		brush->reset();
		return;
	}
	if (beginBrushFromPanel()) {
		_sceneMgr->nodeForeachGroup([&](int nodeId) {
			if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
				if (!node->visible()) {
					return;
				}
				auto callback = [&](const voxel::Region &region, ModifierType modType, SceneModifiedFlags flags) {
					_sceneMgr->modified(nodeId, region, flags);
				};
				execute(_sceneMgr->sceneGraph(), *node, callback);
			}
		});
		endBrush();
	}
	brush->reset();
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
	return isMode(ModifierType::ExistingVoxelMask);
}

BrushType Modifier::setBrushType(BrushType type) {
	if (_brushType == type) {
		return _brushType;
	}

	// Auto-commit pending changes from the current brush before switching.
	// Must happen before changing _brushType so currentBrush() returns the old brush.
	commit();
	resetPreview();

	_brushType = type;
	Brush *newBrush = currentBrush();
	if (newBrush) {
		setModifierType(newBrush->modifierType(_brushContext.modifierType));
		newBrush->onActivated();
	}
	return _brushType;
}

void Modifier::setGridResolution(int gridSize) {
	_brushContext.gridResolution = core_max(1, gridSize);
}

ModifierType Modifier::setModifierType(ModifierType type) {
	if (_brushType != BrushType::None) {
		_brushContext.modifierType = currentBrush()->modifierType(type);
	} else {
		_brushContext.modifierType = type;
	}
	return _brushContext.modifierType;
}

void Modifier::resetPreview() {
	_previewManager.resetPreview();
}

ModifierType Modifier::checkModifierType() {
	if (_brushType == BrushType::None) {
		return ModifierType::ColorPicker;
	}
	return currentBrush()->modifierType(ModifierType::Mask);
}

void Modifier::render(const video::Camera &camera, palette::Palette &activePalette, const glm::mat4 &model) {
	if (_locked) {
		return;
	}

	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNodeId = sceneGraph.activeNode();
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

	// Mirror plane info
	if (brush) {
		ctx.mirrorAxis = brush->mirrorAxis();
		ctx.mirrorPos = brush->mirrorPos();
	}
	if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNodeId)) {
		ctx.activeRegion = node->region();
	}

	// Handle brush preview with deferred updates
	if (!isMode(ModifierType::ColorPicker)) {
		ctx.brushActive = brush && brush->active();
		// Bootstrap: TransformBrush/ExtrudeBrush need preview before internal state exists
		if (!ctx.brushActive && brush &&
			(brush->type() == BrushType::Transform || brush->type() == BrushType::Extrude || brush->type() == BrushType::Sculpt)) {
			ctx.brushActive = true;
		}
	}

	if (ctx.brushActive) {
		if (brush->dirty()) {
			// Clear stale preview so the old position does not keep
			// rendering, then regenerate immediately at the new position.
			resetPreview();
			brush->markClean();
			if (brush->hasPendingChanges()) {
				// Brushes with pending changes (TransformBrush, ExtrudeBrush, SculptBrush)
				// modify the real volume directly. Re-execute on the real node so the
				// updated transform is applied to the scene - generating a separate preview
				// overlay would corrupt brush history state and produce ghost meshes.
				scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNodeId);
				if (node) {
					voxel::RawVolume *activeVolume = node->volume();
					preExecuteBrush(activeVolume);
					executeBrush(sceneGraph, *node, _brushContext.modifierType, _brushContext.cursorVoxel,
						[this, activeNodeId](const voxel::Region &region, ModifierType, SceneModifiedFlags flags) {
							_sceneMgr->modified(activeNodeId, region, flags);
						});
				}
			} else {
				_previewManager.scheduleUpdate(_nowSeconds);
			}
		}
		voxel::RawVolume *activeVolume = _sceneMgr->volume(activeNodeId);
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
		if (ctx.previewVolume || ctx.useSimplePreview) {
			ctx.palette = &activePalette;
		}
	} else {
		// Clear cached preview state when the brush is not active to prevent stale
		// previews from reappearing as ghosts when the brush becomes active again
		resetPreview();
	}

	// Let the renderer handle buffer updates and rendering
	_modifierRenderer->update(ctx);

	// Render everything
	_modifierRenderer->render(camera, model);
}

} // namespace voxedit
