/**
 * @file
 */

#include "ModifierButton.h"
#include "core/BindingContext.h"
#include "../SceneManager.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

ModifierButton::ModifierButton(ModifierType newType) :
		_newType(newType) {
}

bool ModifierButton::handleDown(int32_t key, double pressedMillis) {
	const bool initialDown = Super::handleDown(key, pressedMillis);
	// scene mode
	if (core::bindingContext() == core::BindingContext::Context1) {
		return initialDown;
	}
	if (_secondAction) {
		execute(false);
		return initialDown;
	}
	Modifier& mgr = sceneMgr().modifier();
	if (initialDown) {
		if (_newType != ModifierType::None) {
			_oldType = mgr.modifierType();
			mgr.setModifierType(_newType);
			sceneMgr().trace(true);
		}
		mgr.aabbStart();
	}
	return initialDown;
}

bool ModifierButton::handleUp(int32_t key, double releasedMillis) {
	const bool allUp = Super::handleUp(key, releasedMillis);
	if (_secondAction) {
		_secondAction = false;
		return allUp;
	}
	if (allUp) {
		Modifier& mgr = sceneMgr().modifier();
		_secondAction = mgr.needsSecondAction();
		if (_secondAction) {
			mgr.aabbStep();
			return allUp;
		}
		execute(false);
	} else {
		Log::debug("Not all modifier keys were released - skipped action execution");
	}
	return allUp;
}

void ModifierButton::execute(bool single) {
	Modifier& modifier = sceneMgr().modifier();
	int nodes = 0;
	auto func = [&] (int nodeId) {
		if (voxelformat::SceneGraphNode *node = sceneMgr().sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			Log::debug("Execute modifier action for node %i", nodeId);
			voxel::RawVolume* volume = sceneMgr().volume(nodeId);
			modifier.aabbAction(volume, [&] (const voxel::Region& region, ModifierType type, bool markUndo) {
				if (type != ModifierType::Select && type != ModifierType::ColorPicker) {
					sceneMgr().modified(nodeId, region, markUndo);
				}
			});
			++nodes;
		}
	};
	sceneMgr().nodeForeachGroup(func);
	if (_oldType != ModifierType::None) {
		modifier.setModifierType(_oldType);
		sceneMgr().trace(true);
		_oldType = ModifierType::None;
	}
	if (!single) {
		modifier.aabbAbort();
	}
	if (nodes == 0) {
		Log::warn("Could not execute the desired action on any visible node");
	}
}

}
