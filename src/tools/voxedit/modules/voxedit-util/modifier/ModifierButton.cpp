/**
 * @file
 */

#include "ModifierButton.h"
#include "../SceneManager.h"
#include "core/BindingContext.h"
#include "core/Log.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

ModifierButton::ModifierButton(SceneManager *sceneMgr, ModifierType newType) : _sceneMgr(sceneMgr), _newType(newType) {
}

bool ModifierButton::handleDown(int32_t key, double pressedMillis) {
	const bool initialDown = Super::handleDown(key, pressedMillis);
	// scene mode
	if (core::bindingContext() == core::BindingContext::Context1) {
		return initialDown;
	}
	Modifier &modifier = _sceneMgr->modifier();
	if (_furtherAction && !modifier.aborted()) {
		execute(false);
		return initialDown;
	}
	if (initialDown) {
		if (_newType != ModifierType::None) {
			_oldType = modifier.modifierType();
			modifier.setModifierType(_newType);
			_sceneMgr->trace(false, true);
		}
		modifier.beginBrush();
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
		_furtherAction = modifier.needsAdditionalAction();
		if (_furtherAction) {
			modifier.executeAdditionalAction();
			return allUp;
		}
		execute(false);
	} else {
		Log::trace("Not all modifier keys were released - skipped action execution for %i", (int)_newType);
	}
	return allUp;
}

void ModifierButton::execute(bool single) {
	Modifier &modifier = _sceneMgr->modifier();
	int nodes = 0;
	auto func = [&](int nodeId) {
		if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			Log::debug("Execute modifier action for node %i", nodeId);
			voxel::RawVolume *v = _sceneMgr->volume(nodeId);
			if (v == nullptr) {
				return;
			}
			auto modifierFunc = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
				if (isModifying(type)) {
					_sceneMgr->modified(nodeId, region, flags);
				}
			};
			modifier.execute(_sceneMgr->sceneGraph(), *node, modifierFunc);
			++nodes;
		}
	};
	_sceneMgr->nodeForeachGroup(func);
	if (_oldType != ModifierType::None) {
		modifier.setModifierType(_oldType);
		_sceneMgr->trace(false, true);
		_oldType = ModifierType::None;
	}
	if (!single) {
		modifier.endBrush();
	}
	if (nodes == 0) {
		Log::warn("Could not execute the desired action on any visible node");
	}
}

} // namespace voxedit
