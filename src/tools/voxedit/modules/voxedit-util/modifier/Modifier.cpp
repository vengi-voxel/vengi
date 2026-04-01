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
#include "voxedit-util/ModelNodeSettings.h"
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
	: _stampBrush(sceneMgr), _textureBrush(sceneMgr), _actionExecuteButton(sceneMgr),
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
	// Note: LuaBrush instances are added dynamically during init() via discoverBrushScripts()
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
			reloadBrushScripts();
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

	const core::VarDef voxEditMaxSuggestedVolumeSizePreview(cfg::VoxEditMaxSuggestedVolumeSizePreview, 32, 16, (int)voxedit::MaxVolumeSize, N_("Max preview size"), N_("The maximum size of the preview volume"), -1);
	_maxSuggestedVolumeSizePreview = core::Var::registerVar(voxEditMaxSuggestedVolumeSizePreview);

	const core::VarDef voxEditAutoSelect(cfg::VoxEditAutoSelect, false, N_("Auto select added"), N_("Automatically select newly placed voxels"));
	_autoSelect = core::Var::registerVar(voxEditAutoSelect);

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
	discoverBrushScripts();
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
	clearBrushScripts();
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
	ModifierVolumeWrapper wrapper(node, modifierType);
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

void Modifier::discoverBrushScripts() {
	const io::FilesystemPtr &filesystem = io::filesystem();
	core::DynamicArray<io::FilesystemEntry> entities;
	filesystem->list("brushes", entities, "*.lua");
	for (const auto &e : entities) {
		LuaBrush *brush = new LuaBrush(filesystem);
		if (!brush->init()) {
			Log::error("Failed to initialize lua brush");
			delete brush;
			continue;
		}
		if (!brush->loadScript(e.name)) {
			Log::warn("Failed to load brush script: %s", e.name.c_str());
			brush->shutdown();
			delete brush;
			continue;
		}
		_luaBrushes.push_back(brush);
		Log::info("Discovered brush script: %s", e.name.c_str());
	}
	if (!_luaBrushes.empty()) {
		_activeLuaBrushIndex = 0;
	}
}

void Modifier::clearBrushScripts() {
	for (LuaBrush *b : _luaBrushes) {
		b->shutdown();
		delete b;
	}
	_luaBrushes.clear();
	_activeLuaBrushIndex = -1;
}

void Modifier::reloadBrushScripts() {
	clearBrushScripts();
	discoverBrushScripts();
	Log::debug("Reloaded brush scripts (%i found)", (int)_luaBrushes.size());
}

LuaBrush *Modifier::activeLuaBrush() {
	if (_activeLuaBrushIndex >= 0 && _activeLuaBrushIndex < (int)_luaBrushes.size()) {
		return _luaBrushes[_activeLuaBrushIndex];
	}
	return nullptr;
}

const LuaBrush *Modifier::activeLuaBrush() const {
	if (_activeLuaBrushIndex >= 0 && _activeLuaBrushIndex < (int)_luaBrushes.size()) {
		return _luaBrushes[_activeLuaBrushIndex];
	}
	return nullptr;
}

void Modifier::setActiveLuaBrushIndex(int index) {
	if (index >= 0 && index < (int)_luaBrushes.size()) {
		_activeLuaBrushIndex = index;
	}
}

Brush *Modifier::currentBrush() {
	if (_brushType == BrushType::Script) {
		return activeLuaBrush();
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
		return activeLuaBrush();
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

void Modifier::brushApply() {
	Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	if (!brush->onDeactivated()) {
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
	Brush *oldBrush = currentBrush();
	if (oldBrush && oldBrush->onDeactivated()) {
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
		oldBrush->reset();
	}

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

ModifierType Modifier::checkModifierType() {
	if (_brushType == BrushType::None) {
		return ModifierType::ColorPicker;
	}
	return currentBrush()->modifierType(ModifierType::Mask);
}

static void createOrClearPreviewVolume(voxel::RawVolume *existingVolume, core::ScopedPtr<voxel::RawVolume> &volume,
									   voxel::Region region) {
	if (existingVolume == nullptr) {
		if (volume == nullptr || volume->region() != region) {
			volume = new voxel::RawVolume(region);
			return;
		}
		volume->clear();
	} else {
		// Keep the old volume alive during allocation so the heap allocator
		// cannot reuse the same address. MeshState::setVolume() compares raw
		// pointers (old == new) and skips the update when they match, which
		// would leave stale mesh data in GPU buffers.
		voxel::RawVolume *old = volume.release();
		volume = new voxel::RawVolume(*existingVolume, region);
		delete old;
	}
}

static bool canAllocatePreviewRegion(const voxel::Region &region, int maxSuggestedExtent) {
	if (!region.isValid()) {
		return false;
	}
	const glm::ivec3 &dimensions = region.getDimensionsInVoxels();
	if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
		return false;
	}
	const int64_t maxExtent = (int64_t)maxSuggestedExtent;
	const int64_t maxVoxels = maxExtent * maxExtent * maxExtent;
	const int64_t voxels = (int64_t)dimensions.x * (int64_t)dimensions.y * (int64_t)dimensions.z;
	return voxels > 0 && voxels <= maxVoxels;
}

bool Modifier::previewNeedsExistingVolume() const {
	if (isMode(ModifierType::Paint)) {
		return true;
	}
	if (_brushType == BrushType::Select) {
		return true;
	}
	if (_brushType == BrushType::Plane) {
		return isMode(ModifierType::Place);
	}
	if (_brushType == BrushType::Extrude) {
		return true;
	}
	if (_brushType == BrushType::Transform) {
		return true;
	}
	if (_brushType == BrushType::Sculpt) {
		return true;
	}
	return false;
}

bool Modifier::isSimplePreview(const Brush *brush, const voxel::Region &region) const {
	if (brush->type() == BrushType::Script) {
		const LuaBrush *luaBrush = (const LuaBrush *)brush;
		if (luaBrush->useSimplePreview()) {
			return true;
		}
	}
	if (brush->type() == BrushType::Shape) {
		const ShapeBrush *shapeBrush = (const ShapeBrush *)brush;
		if (shapeBrush->shapeType() == ShapeType::AABB) {
			return true;
		}
	}
	if (brush->type() == BrushType::Select) {
		const SelectBrush *selectBrush = (const SelectBrush *)brush;
		const SelectMode mode = selectBrush->selectMode();
		if (mode == SelectMode::All || mode == SelectMode::Surface || mode == SelectMode::Box3D ||
			mode == SelectMode::SameColor || mode == SelectMode::FuzzyColor) {
			return true;
		}
	}
	if (brush->type() == BrushType::Sculpt) {
		const SculptBrush *sculptBrush = (const SculptBrush *)brush;
		if (sculptBrush->sculptMode() == SculptMode::ExtendPlane && sculptBrush->planeFitted()) {
			return true;
		}
	}
	return false;
}

void Modifier::resetPreview() {
	// Clear renderer volume pointers before freeing the volumes.
	// The allocator may reuse the same heap address for the next preview volume,
	// which would cause MeshState::setVolume() to see old == new and skip the
	// update, leaving stale mesh data in GPU buffers.
	_modifierRenderer->clearBrushVolumes();
	_previewVolume = nullptr;
	_previewMirrorVolume = nullptr;
	_brushPreview = BrushPreview();
}

void Modifier::updateBrushVolumePreview(palette::Palette &activePalette, voxel::RawVolume *activeVolume,
										scenegraph::SceneGraph &sceneGraph) {
	// even in erase mode we want the preview to create the models, not wipe them
	ModifierType modifierType = _brushContext.modifierType;
	if (modifierType == ModifierType::Erase) {
		modifierType = ModifierType::Place;
	}
	voxel::Voxel voxel = _brushContext.cursorVoxel;
	voxel.setOutline();

	// Allow subclasses to wait for pending operations before freeing old preview volumes
	_modifierRenderer->waitForPendingExtractions();

	// Reset preview state
	resetPreview();

	Log::debug("regenerate preview volume");

	if (activeVolume == nullptr) {
		return;
	}

	// operate on existing voxels
	voxel::RawVolume *existingVolume = nullptr;
	if (previewNeedsExistingVolume()) {
		existingVolume = activeVolume;
	}

	const Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	preExecuteBrush(activeVolume);
	const voxel::Region &region = brush->calcRegion(_brushContext);
	if (!region.isValid()) {
		return;
	}
	const int maxPreviewExtent = _maxSuggestedVolumeSizePreview->intVal();
	bool simplePreview = isSimplePreview(brush, region);
	if (!simplePreview && canAllocatePreviewRegion(region, maxPreviewExtent)) {
		const bool isCircleSelect = _brushType == BrushType::Select &&
			(_selectBrush.selectMode() == SelectMode::Circle ||
			 _selectBrush.selectMode() == SelectMode::Lasso);
		if (isCircleSelect) {
			_selectBrush.setPreviewMode(true);
		}
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			createOrClearPreviewVolume(existingVolume, _previewMirrorVolume, voxel::Region(minsMirror, maxsMirror));
			scenegraph::SceneGraphNode mirrorDummyNode(scenegraph::SceneGraphNodeType::Model);
			mirrorDummyNode.setUnownedVolume(_previewMirrorVolume);
			executeBrush(sceneGraph, mirrorDummyNode, modifierType, voxel);
		}
		createOrClearPreviewVolume(existingVolume, _previewVolume, region);
		scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
		dummyNode.setUnownedVolume(_previewVolume);
		executeBrush(sceneGraph, dummyNode, modifierType, voxel);
		if (isCircleSelect) {
			_selectBrush.setPreviewMode(false);
		}
	} else if (simplePreview) {
		_brushPreview.useSimplePreview = true;
		_brushPreview.simplePreviewRegion = region;
		_brushPreview.simplePreviewColor = activePalette.color(_brushContext.cursorVoxel.getColor());
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			_brushPreview.simpleMirrorPreviewRegion = voxel::Region(minsMirror, maxsMirror);
		}
	}
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
			_nextPreviewUpdateSeconds = _nowSeconds;
			brush->markClean();
		}
		if (_nextPreviewUpdateSeconds > 0.0) {
			if (_nextPreviewUpdateSeconds <= _nowSeconds) {
				_nextPreviewUpdateSeconds = 0.0;
				voxel::RawVolume *activeVolume = _sceneMgr->volume(activeNodeId);
				updateBrushVolumePreview(activePalette, activeVolume, sceneGraph);
			}
		}
		// Copy cached preview state to renderer context
		ctx.previewVolume = previewVolume();
		ctx.previewMirrorVolume = previewMirrorVolume();
		const BrushPreview &preview = brushPreview();
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
