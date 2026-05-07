/**
 * @file
 */

#include "SelectBrush.h"

namespace voxedit {

SelectBrush::SelectBrush(SceneManager *sceneManager)
	: Super(BrushType::Select, ModifierType::Override, ModifierType::Override | ModifierType::Erase),
	  _sceneManager(sceneManager), _lassoStrategy(sceneManager), _scriptStrategy(sceneManager) {
	setBrushClamping(true);
	_strategies[(int)SelectMode::All] = &_selectAll;
	_strategies[(int)SelectMode::Surface] = &_selectSurface;
	_strategies[(int)SelectMode::SameColor] = &_selectSameColor;
	_strategies[(int)SelectMode::FuzzyColor] = &_fuzzyColorStrategy;
	_strategies[(int)SelectMode::Connected] = &_selectConnected;
	_strategies[(int)SelectMode::FlatSurface] = &_flatSurfaceStrategy;
	_strategies[(int)SelectMode::Box3D] = &_box3DStrategy;
	_strategies[(int)SelectMode::Circle] = &_circleStrategy;
	_strategies[(int)SelectMode::Lasso] = &_lassoStrategy;
	_strategies[(int)SelectMode::Paint] = &_paintStrategy;
	_strategies[(int)SelectMode::Script] = &_scriptStrategy;
}

select::AABBBrushState SelectBrush::buildState(const BrushContext &ctx) const {
	select::AABBBrushState state;
	state.aabbMode = _aabbMode;
	state.aabbFace = _aabbFace;
	state.aabbFirstPos = applyGridResolution(_aabbFirstPos, ctx.gridResolution);
	state.cursorPosition = currentCursorPosition(ctx);
	state.radius = radius();
	return state;
}

bool SelectBrush::active() const {
	if (activeStrategy()->active()) {
		return true;
	}
	return Super::active();
}

void SelectBrush::reset() {
	Super::reset();
	for (int i = 0; i < (int)SelectMode::Max; ++i) {
		_strategies[i]->reset();
	}
	_sceneModifiedFlags = SceneModifiedFlags::All;
}

void SelectBrush::onSceneChange() {
	Super::onSceneChange();
	reset();
}

void SelectBrush::abort(BrushContext &ctx) {
	activeStrategy()->abort(ctx);
	_sceneModifiedFlags = activeStrategy()->_modifiedFlags;
	Super::abort(ctx);
}

bool SelectBrush::hasPendingChanges() const {
	return _paintStrategy.hasPendingChanges();
}

voxel::Region SelectBrush::revertChanges(voxel::RawVolume *) {
	return voxel::Region::InvalidRegion;
}

void SelectBrush::endBrush(BrushContext &ctx) {
	activeStrategy()->endBrush(ctx);
	_sceneModifiedFlags = activeStrategy()->_modifiedFlags;
	Super::endBrush(ctx);
}

voxel::Region SelectBrush::consumePendingUndoRegion() {
	return _paintStrategy.consumeFinalUndoRegion();
}

void SelectBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	activeStrategy()->update(ctx, nowSeconds);
}

void SelectBrush::setSelectMode(SelectMode mode) {
	if (_selectMode != mode) {
		if (_selectMode == SelectMode::Paint) {
			setAABBMode();
			_sceneModifiedFlags = SceneModifiedFlags::All;
		}
		if (_selectMode == SelectMode::Lasso || _selectMode == SelectMode::Paint || _selectMode == SelectMode::Circle ||
			_selectMode == SelectMode::Box3D) {
			activeStrategy()->reset();
		}
		if (mode == SelectMode::Paint) {
			setSingleMode();
			if (_radius == 0) {
				setRadius(1);
			}
		}
	}
	_selectMode = mode;
}

void SelectBrush::setLuaSelectionMode(int index, LUASelectionMode *mode) {
	_luaSelectionModeIndex = index;
	_scriptStrategy.setActiveLuaMode(mode);
	if (index >= 0 && mode != nullptr) {
		_selectMode = SelectMode::Script;
	} else if (_selectMode == SelectMode::Script) {
		_selectMode = SelectMode::All;
	}
}

bool SelectBrush::needsAdditionalAction(const BrushContext &ctx) const {
	if (activeStrategy()->needsAdditionalAction(ctx)) {
		return Super::needsAdditionalAction(ctx);
	}
	return false;
}

bool SelectBrush::beginBrush(const BrushContext &ctx) {
	const select::AABBBrushState state = buildState(ctx);
	if (activeStrategy()->beginBrush(ctx, state)) {
		_sceneModifiedFlags = activeStrategy()->_modifiedFlags;
		// TODO: SELECTION: why only in lasso - this class should not know this. ideally the strategy is either doing
		// this in beginBrush() and is triggered by the SelectBrush implementation or it's done for all strategies,
		// because this class should not decide on that
		if (_selectMode == SelectMode::Lasso) {
			_aabbFace = ctx.cursorFace != voxel::FaceNames::Max ? ctx.cursorFace : voxel::FaceNames::PositiveY;
		}
		return true;
	}
	return Super::beginBrush(ctx);
}

bool SelectBrush::isSimplePreview() const {
	return activeStrategy()->isSimplePreview();
}

voxel::Region SelectBrush::calcRegion(const BrushContext &ctx) const {
	const select::AABBBrushState state = buildState(ctx);
	const voxel::Region strategyRegion = activeStrategy()->calcRegion(ctx, state);
	if (strategyRegion.isValid()) {
		return strategyRegion;
	}
	// TODO: SELECTION: this is a bit hacky - we should ideally have the strategies return the region in activeStrategy()->calcRegion()
	if (_selectMode != SelectMode::All && _selectMode != SelectMode::Box3D && _selectMode != SelectMode::Paint) {
		return ctx.targetVolumeRegion;
	}
	return Super::calcRegion(ctx);
}

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	voxel::Region selectionRegion = region;
	if (_brushClamping) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}
	// TODO: SELECTION: this reset is hacky - why is it here? if needed - at least document why.
	_box3DStrategy.reset();
	const select::AABBBrushState state = buildState(ctx);
	activeStrategy()->generate(sceneGraph, wrapper, ctx, selectionRegion, state);
	_sceneModifiedFlags = activeStrategy()->_modifiedFlags;
}

bool SelectBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return activeStrategy()->wantBrushGizmo(ctx);
}

void SelectBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	activeStrategy()->brushGizmoState(ctx, state);
}

} // namespace voxedit
