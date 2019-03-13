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
	_layers.clear();
	_statePosition = 0u;
}

std::pair<int, voxel::RawVolume*> MementoHandler::undo() {
	if (!canUndo()) {
		return std::make_pair(-1, nullptr);
	}
	core_assert(_statePosition >= 1);
	--_statePosition;
	std::pair<int, voxel::RawVolume*> s = state();
	return std::make_pair(s.first, new voxel::RawVolume(s.second));
}

std::pair<int, voxel::RawVolume*> MementoHandler::redo() {
	if (!canRedo()) {
		return std::make_pair(-1, nullptr);
	}
	++_statePosition;
	std::pair<int, voxel::RawVolume*> s = state();
	return std::make_pair(s.first, new voxel::RawVolume(s.second));
}

void MementoHandler::markUndo(int layer, const voxel::RawVolume* volume) {
	if (!_states.empty()) {
		auto iStates = _states.begin();
		std::advance(iStates, _statePosition + 1);
		for (auto iter = iStates; iter < _states.end(); ++iter) {
			delete *iter;
		}
		_states.erase(iStates, _states.end());
		auto iLayers = _layers.begin();
		std::advance(iLayers, _statePosition + 1);
		_layers.erase(iLayers, _layers.end());
	}
	_states.push_back(new voxel::RawVolume(volume));
	_layers.push_back(layer);
	while (_states.size() > _maxStates) {
		delete *_states.begin();
		_states.erase(_states.begin());
		_layers.erase(_layers.begin());
	}
	_statePosition = stateSize() - 1;
}

}
