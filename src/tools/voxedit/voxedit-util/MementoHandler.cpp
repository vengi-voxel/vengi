/**
 * @file
 */

#include "MementoHandler.h"

#include "voxel/polyvox/RawVolume.h"
#include "core/Assert.h"

namespace voxedit {

static const LayerState InvalidLayerState{nullptr, -1};

// TODO: undo layer create/delete...
MementoHandler::MementoHandler() {
	_states.reserve(_maxStates);
}

MementoHandler::~MementoHandler() {
	clearStates();
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
	const LayerState& s = state();
	return LayerState{new voxel::RawVolume(s.volume), s.layer};
}

LayerState MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidLayerState;
	}
	++_statePosition;
	const LayerState& s = state();
	return LayerState{new voxel::RawVolume(s.volume), s.layer};
}

void MementoHandler::markUndo(int layer, const voxel::RawVolume* volume) {
	if (!_states.empty()) {
		auto iStates = _states.begin();
		std::advance(iStates, _statePosition + 1);
		for (auto iter = iStates; iter < _states.end(); ++iter) {
			delete iter->volume;
		}
		_states.erase(iStates, _states.end());
	}
	_states.push_back(LayerState{new voxel::RawVolume(volume), layer});
	while (_states.size() > _maxStates) {
		delete _states.begin()->volume;
		_states.erase(_states.begin());
	}
	_statePosition = stateSize() - 1;
}

}
