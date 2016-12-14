#include "UndoHandler.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxedit {

UndoHandler::UndoHandler() {
	_undoStates.reserve(_maxUndoStates);
}

UndoHandler::~UndoHandler() {
	clearUndoStates();
}

void UndoHandler::clearUndoStates() {
	for (voxel::RawVolume* vol : _undoStates) {
		delete vol;
	}
	_undoStates.clear();
	_undoPosition = 0u;
}

voxel::RawVolume* UndoHandler::undo() {
	if (!canUndo()) {
		return nullptr;
	}
	return new voxel::RawVolume(_undoStates[--_undoPosition]);
}

voxel::RawVolume* UndoHandler::redo() {
	if (!canRedo()) {
		return nullptr;
	}
	return new voxel::RawVolume(_undoStates[_undoPosition++]);
}

void UndoHandler::markUndo(const voxel::RawVolume* volume) {
	auto i = _undoStates.begin();
	std::advance(i, _undoPosition);
	for (auto iter = i; iter < _undoStates.end(); ++iter) {
		delete *iter;
	}
	_undoStates.erase(i, _undoStates.end());
	_undoStates.push_back(new voxel::RawVolume(volume));
	while (_undoStates.size() > _maxUndoStates) {
		delete *_undoStates.begin();
		_undoStates.erase(_undoStates.begin());
	}
	_undoPosition = _undoStates.size();
}

}
