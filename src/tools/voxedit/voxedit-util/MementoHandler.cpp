/**
 * @file
 */

#include "MementoHandler.h"

#include "voxel/polyvox/RawVolume.h"
#include "core/command/Command.h"
#include "core/Assert.h"
#include "core/Log.h"

namespace voxedit {

static const LayerState InvalidLayerState{nullptr, -1, ""};
const int MementoHandler::MaxStates = 64;

MementoHandler::MementoHandler() {
}

MementoHandler::~MementoHandler() {
	shutdown();
}

bool MementoHandler::init() {
	_states.reserve(MaxStates);
	return true;
}

void MementoHandler::shutdown() {
	clearStates();
}

void MementoHandler::lock() {
	++_locked;
}

void MementoHandler::unlock() {
	--_locked;
}

void MementoHandler::construct() {
	core::Command::registerCommand("ve_mementoinfo", [&] (const core::CmdArgs& args) {
		Log::info("Current memento state index: %i", _statePosition);
		Log::info("Maximum memento states: %i", MaxStates);
		int i = 0;
		for (LayerState& state : _states) {
			Log::info("%4i: %i - %s (%s)", i++, state.layer, state.name.c_str(), state.volume == nullptr ? "empty" : "volume");
		}
	});
}

void MementoHandler::clearStates() {
	for (LayerState& state : _states) {
		delete state.volume;
	}
	_states.clear();
	_statePosition = 0u;
}

LayerState MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidLayerState;
	}
	core_assert(_statePosition >= 1);
	--_statePosition;
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const LayerState& s = state();
	return LayerState{new voxel::RawVolume(s.volume), s.layer, s.name};
}

LayerState MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidLayerState;
	}
	++_statePosition;
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const LayerState& s = state();
	return LayerState{new voxel::RawVolume(s.volume), s.layer, s.name};
}

void MementoHandler::markUndo(int layer, const std::string& name, const voxel::RawVolume* volume) {
	if (!_states.empty()) {
		auto iStates = _states.begin();
		std::advance(iStates, _statePosition + 1);
		for (auto iter = iStates; iter < _states.end(); ++iter) {
			delete iter->volume;
		}
		_states.erase(iStates, _states.end());
	}
	Log::debug("New undo state for layer %i with name %s (memento state index: %i)", layer, name.c_str(), (int)_states.size());
	_states.push_back(LayerState{new voxel::RawVolume(volume), layer, name});
	while (_states.size() > MaxStates) {
		delete _states.begin()->volume;
		_states.erase(_states.begin());
	}
	_statePosition = stateSize() - 1;
}

}
