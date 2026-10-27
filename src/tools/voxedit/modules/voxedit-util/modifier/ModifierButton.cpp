/**
 * @file
 */

#include "ModifierButton.h"
#include "Modifier.h"
#include "../SceneManager.h"
#include "brush/Brush.h"
#include "core/BindingContext.h"
#include "core/Log.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

ModifierButton::ModifierButton(SceneManager *sceneMgr, ModifierType newType) : _sceneMgr(sceneMgr), _newType(newType) {
}

bool ModifierButton::handleDown(int32_t key, double pressedMillis) {
	const bool wasPressed = pressed();
	const bool initialDown = Super::handleDown(key, pressedMillis);
	if (wasPressed) {
		return initialDown;
	}
	// scene mode (including scene+gizmo)
	if ((core::bindingContext() & core::BindingContext::Context1) != 0) {
		return initialDown;
	}
	Modifier &modifier = _sceneMgr->modifier();
	_sliceAction = modifier.isSliceMode();
	// Gizmo hover is latched from the previous viewport frame. A click on the
	// extrude/transform handle must not start a brush action or pick a new face
	if (_sceneMgr->isViewportGizmoActive() || modifier.isBrushGizmoActive()) {
		return initialDown;
	}
	if (_furtherAction && !modifier.aborted()) {
		execute(false);
		return initialDown;
	}
	if (initialDown) {
		if (_newType != ModifierType::None) {
			_oldType = modifier.modifierType();
			modifier.setModifierType(_newType);
			if (!_sliceAction) {
				_sceneMgr->trace(false, true);
			}
		}
		modifier.beginBrush();
		// Stroke mode paints continuously while held; apply the first dab immediately
		// so click-and-drag does not wait for the next Modifier::update tick.
		if (Brush *brush = modifier.currentBrush()) {
			if (brush->executeOnPress()) {
				execute(true);
			}
		}
	}
	return initialDown;
}

bool ModifierButton::handleUp(int32_t key, double releasedMillis) {
	const bool allUp = Super::handleUp(key, releasedMillis);
	if (_furtherAction) {
		_furtherAction = false;
		return allUp;
	}
	if (allUp) {
		Modifier &modifier = _sceneMgr->modifier();
		if (_sceneMgr->isViewportGizmoActive() || modifier.isBrushGizmoActive()) {
			modifier.endBrush();
			_sliceAction = false;
			return allUp;
		}
		_furtherAction = modifier.needsAdditionalAction();
		if (_furtherAction) {
			modifier.executeAdditionalAction();
			return allUp;
		}
		execute(false);
		_sliceAction = false;
	} else {
		Log::trace("Not all modifier keys were released - skipped action execution for %i", (int)_newType);
	}
	return allUp;
}

void ModifierButton::execute(bool continuous) {
	Modifier &modifier = _sceneMgr->modifier();
	if (modifier.isBrushGizmoActive()) {
		return;
	}
	const bool previousSliceMode = modifier.isSliceMode();
	if (_sliceAction) {
		modifier.setSliceMode(true);
	}
	int nodes = 0;
	auto func = [&](scenegraph::SceneGraphNode &node) {
		if (!node.visible()) {
			return;
		}
		Log::debug("Execute modifier action for node %i", node.id());
		voxel::RawVolume *v = _sceneMgr->volume(node.uuid());
		if (v == nullptr) {
			return;
		}
		auto modifierFunc = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
			_sceneMgr->modified(node.uuid(), region, flags);
		};
		modifier.execute(_sceneMgr->sceneGraph(), node, modifierFunc);
		++nodes;
	};
	if (_sliceAction) {
		if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode())) {
			if (!node->locked()) {
				func(*node);
			}
		}
	} else {
		_sceneMgr->nodeForeachGroup(func);
	}
	if (!continuous && _oldType != ModifierType::None) {
		modifier.setModifierType(_oldType);
		if (!_sliceAction) {
			_sceneMgr->trace(false, true);
		}
		_oldType = ModifierType::None;
	}
	if (!continuous) {
		Brush *brush = modifier.currentBrush();
		modifier.endBrush();
		if (brush) {
			voxel::Region pendingRegion = brush->consumePendingUndoRegion();
			if (pendingRegion.isValid()) {
				auto undoFunc = [&](const core::UUID &nodeUUID) {
					_sceneMgr->modified(nodeUUID, pendingRegion, SceneModifiedFlags::MarkUndo);
				};
				if (_sliceAction) {
					undoFunc(_sceneMgr->activeNodeUUID());
				} else {
					_sceneMgr->nodeForeachGroup(undoFunc);
				}
			}
		}
	}
	modifier.setSliceMode(previousSliceMode);
	if (nodes == 0) {
		Log::warn("Could not execute the desired action on any visible node");
	}
}

} // namespace voxedit
