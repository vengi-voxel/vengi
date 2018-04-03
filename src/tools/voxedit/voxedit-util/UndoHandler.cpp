/**
 * @file
 */

#include "UndoHandler.h"
#include "voxel/polyvox/RawVolume.h"
#include "core/Assert.h"

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
	core_assert(_undoPosition >= 1);
	--_undoPosition;
	voxel::RawVolume* v = undoState();
	return new voxel::RawVolume(v);
}

voxel::RawVolume* UndoHandler::redo() {
	if (!canRedo()) {
		return nullptr;
	}
	++_undoPosition;
	voxel::RawVolume* v = undoState();
	return new voxel::RawVolume(v);
}

void UndoHandler::markUndo(const voxel::RawVolume* volume) {
	if (!_undoStates.empty()) {
		auto i = _undoStates.begin();
		std::advance(i, _undoPosition + 1);
		for (auto iter = i; iter < _undoStates.end(); ++iter) {
			delete *iter;
		}
		_undoStates.erase(i, _undoStates.end());
	}
	_undoStates.push_back(new voxel::RawVolume(volume));
	while (_undoStates.size() > _maxUndoStates) {
		delete *_undoStates.begin();
		_undoStates.erase(_undoStates.begin());
	}
	_undoPosition = undoSize() - 1;
}

}
