/**
 * @file
 */

#include "MementoHandler.h"

#include "voxel/polyvox/RawVolume.h"
#include "core/Assert.h"

namespace voxedit {

MementoHandler::MementoHandler() {
	_states.reserve(_maxStates);
}

MementoHandler::~MementoHandler() {
	clearStates();
}

void MementoHandler::clearStates() {
	for (voxel::RawVolume* vol : _states) {
		delete vol;
	}
	_states.clear();
	_statePosition = 0u;
}

voxel::RawVolume* MementoHandler::undo() {
	if (!canUndo()) {
		return nullptr;
	}
	core_assert(_statePosition >= 1);
	--_statePosition;
	voxel::RawVolume* v = state();
	return new voxel::RawVolume(v);
}

voxel::RawVolume* MementoHandler::redo() {
	if (!canRedo()) {
		return nullptr;
	}
	++_statePosition;
	voxel::RawVolume* v = state();
	return new voxel::RawVolume(v);
}

void MementoHandler::markUndo(const voxel::RawVolume* volume) {
	if (!_states.empty()) {
		auto i = _states.begin();
		std::advance(i, _statePosition + 1);
		for (auto iter = i; iter < _states.end(); ++iter) {
			delete *iter;
		}
		_states.erase(i, _states.end());
	}
	_states.push_back(new voxel::RawVolume(volume));
	while (_states.size() > _maxStates) {
		delete *_states.begin();
		_states.erase(_states.begin());
	}
	_statePosition = stateSize() - 1;
}

}
