/**
 * @file
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace voxel {
class RawVolume;
}

namespace voxedit {

class UndoHandler {
private:
	std::vector<voxel::RawVolume*> _undoStates;
	uint8_t _undoPosition = 0u;
	static constexpr int _maxUndoStates = 64;

public:
	UndoHandler();
	~UndoHandler();

	void clearUndoStates();
	void markUndo(const voxel::RawVolume* volume);

	voxel::RawVolume* undo();
	voxel::RawVolume* redo();
	bool canUndo() const;
	bool canRedo() const;

	voxel::RawVolume* undoState() const;

	size_t undoSize() const;
	uint8_t undoPosition() const;
};

inline voxel::RawVolume* UndoHandler::undoState() const {
	return _undoStates[_undoPosition];
}

inline uint8_t UndoHandler::undoPosition() const {
	return _undoPosition;
}

inline size_t UndoHandler::undoSize() const {
	return _undoStates.size();
}

inline bool UndoHandler::canUndo() const {
	if (undoSize() <= 1) {
		return false;
	}
	return _undoPosition > 0;
}

inline bool UndoHandler::canRedo() const {
	if (_undoStates.empty()) {
		return false;
	}
	return _undoPosition < undoSize() - 1;
}

}
