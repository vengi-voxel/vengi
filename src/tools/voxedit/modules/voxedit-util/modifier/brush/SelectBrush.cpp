/**
 * @file
 */

#include "SelectBrush.h"

namespace voxedit {

SelectBrush::SelectBrush(SceneManager *sceneManager)
	: Super(BrushType::Select, ModifierType::Override, ModifierType::Override | ModifierType::Erase),
	  _sceneManager(sceneManager), _lassoStrategy(sceneManager), _scriptStrategy(sceneManager) {
	setClampToVolume(true);
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

SelectBrush::SelectBrush(const SelectBrush &other)
	: Super(other), _sceneManager(other._sceneManager), _lassoStrategy(other._sceneManager),
	  _scriptStrategy(other._sceneManager) {
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
	_selectMode = other._selectMode;
	_luaSelectionModeIndex = other._luaSelectionModeIndex;
	_circleStrategy = other._circleStrategy;
	_lassoStrategy = other._lassoStrategy;
	_paintStrategy = other._paintStrategy;
	_box3DStrategy = other._box3DStrategy;
	_fuzzyColorStrategy = other._fuzzyColorStrategy;
	_flatSurfaceStrategy = other._flatSurfaceStrategy;
	_scriptStrategy = other._scriptStrategy;
}

void SelectBrush::syncInput(const Brush &input) {
	Super::syncInput(input);
	const auto &source = (const SelectBrush &)input;
	_lassoStrategy = source._lassoStrategy;
	_circleStrategy = source._circleStrategy;
}

select::AABBBrushState SelectBrush::buildState(const BrushContext &ctx) const {
	select::AABBBrushState state;
	state.boxMode = _boxGesture.active();
	state.aabbFace = _boxGesture.face();
	state.aabbFirstPos = applyGridResolution(_boxGesture.firstPosition(), ctx.gridResolution);
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
}

void SelectBrush::onSceneChange() {
	Super::onSceneChange();
	reset();
}

void SelectBrush::abort(BrushContext &ctx) {
	activeStrategy()->abort(ctx);
	Super::abort(ctx);
}

void SelectBrush::endBrush(BrushContext &ctx) {
	activeStrategy()->endBrush(ctx);
	Super::endBrush(ctx);
}

void SelectBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	activeStrategy()->update(ctx, nowSeconds);
}

void SelectBrush::setSelectMode(SelectMode mode) {
	markSettingsChanged();
	if (_selectMode != mode) {
		if (_selectMode == SelectMode::Paint) {
			setBoxMode();
		}
		activeStrategy()->reset();
		_circleStrategy.reset();
		if (mode == SelectMode::Paint) {
			setStrokeMode();
			if (_radius == 0) {
				setRadius(1);
			}
		}
	}
	_selectMode = mode;
}

void SelectBrush::setLuaSelectionMode(int index, LUASelectionMode *mode) {
	markSettingsChanged();
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
		// When the strategy handles beginBrush itself (e.g. Lasso), Super::beginBrush
		// is not called, so capture the face here for buildState() to work.
		_boxGesture.setFace(ctx.cursorFace != voxel::FaceNames::Max ? ctx.cursorFace : voxel::FaceNames::PositiveY);
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
	// Strategy returned InvalidRegion - use the parent AABB region
	return Super::calcRegion(ctx);
}

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	voxel::Region selectionRegion = region;
	if (_clampToVolume) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}
	// Reset Box3D selection region before each generate so that the previous
	// Box3D selection doesn't persist as a masking region in ModifierVolumeWrapper
	// when a different strategy is active.
	if (_selectMode != SelectMode::Box3D) {
		_box3DStrategy.reset();
	}
	const select::AABBBrushState state = buildState(ctx);
	if (!activeStrategy()->evaluate(sceneGraph, wrapper, ctx, selectionRegion, state)) {
		setErrorReason(N_("Selection script execution failed"));
	}
}

void SelectBrush::handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const {
	if (hasHandle(ctx)) {
		BrushGizmoState handle;
		describeHandle(ctx, handle);
		output.push_back(handle);
	} else {
		Super::handles(ctx, output);
	}
}

bool SelectBrush::hasHandle(const BrushContext &ctx) const {
	return activeStrategy()->hasHandle(ctx, buildState(ctx));
}

void SelectBrush::describeHandle(const BrushContext &ctx, BrushGizmoState &state) const {
	activeStrategy()->describeHandle(ctx, buildState(ctx), state);
}

bool SelectBrush::editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
								  uint32_t operation) {
	if (activeStrategy()->editHandle(handleId, ctx, matrix, deltaMatrix, operation)) {
		markDirty();
		return true;
	}
	return Super::editHandle(handleId, ctx, matrix, deltaMatrix, operation);
}

} // namespace voxedit
