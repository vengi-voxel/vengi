/**
 * @file
 */

#include "ModifierButton.h"
#include "Modifier.h"
#include "../SceneManager.h"
#include "core/BindingContext.h"

namespace voxedit {

ModifierButton::ModifierButton(SceneManager *sceneMgr, ModifierType newType) : _sceneMgr(sceneMgr), _newType(newType) {
}

bool ModifierButton::handleDown(int32_t key, double pressedMillis) {
	const bool wasPressed = pressed();
	const bool initialDown = Super::handleDown(key, pressedMillis);
	if (wasPressed || !initialDown || (core::bindingContext() & core::BindingContext::Context1) != 0) {
		return initialDown;
	}
	_ownsPress = false;
	Modifier &modifier = _sceneMgr->modifier();
	_sliceAction = modifier.isSliceMode();
	if (_sceneMgr->isViewportGizmoActive() || modifier.isBrushGizmoActive()) {
		return initialDown;
	}
	if (modifier.isMode(ModifierType::ColorPicker)) {
		modifier.setCursorVoxel(modifier.hitCursorVoxel());
		return initialDown;
	}
	if (modifier.isAdjustable()) {
		modifier.beginBrush();
		modifier.parametersChanged();
		modifier.endBrush();
		return initialDown;
	}
	if (modifier.isEditActive()) {
		// The next press belongs to the existing multi-step gesture.
		if (_editId == modifier.editId()) {
			_ownsPress = true;
			return initialDown;
		}
	}
	_editId = 0;
	if (!modifier.beginEdit(_newType, _sliceAction)) {
		return initialDown;
	}
	_editId = modifier.editId();
	_ownsPress = true;
	if (const Brush *brush = modifier.currentBrush()) {
			if (brush->executeOnPress()) {
			modifier.updateEdit();
		}
	}
	return initialDown;
}

bool ModifierButton::handleUp(int32_t key, double releasedMillis) {
	const bool allUp = Super::handleUp(key, releasedMillis);
	if (allUp && _ownsPress) {
		Modifier &modifier = _sceneMgr->modifier();
		if (_editId != 0 && _editId == modifier.editId()) {
			modifier.finishEdit(modifier.isBrushGizmoActive() ? BrushEditFinishReason::Interrupt
															  : BrushEditFinishReason::Release);
		}
		if (!modifier.isEditActive() || _editId != modifier.editId()) {
			_editId = 0;
		}
	}
	if (allUp) {
		_sliceAction = false;
		_ownsPress = false;
	}
	return allUp;
}

void ModifierButton::execute(bool continuous) {
	Modifier &modifier = _sceneMgr->modifier();
	if (!_ownsPress || _editId == 0 || _editId != modifier.editId() || modifier.isBrushGizmoActive()) {
		return;
	}
	if (continuous) {
		modifier.updateEdit();
				} else {
		modifier.finishEdit(BrushEditFinishReason::Release);
	}
}

} // namespace voxedit
