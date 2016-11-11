#pragma once

#include <vector>
#include <cstdint>

namespace voxel {
class RawVolume;
}

namespace voxedit {

class UndoHandler {
private:
	std::vector<voxel::RawVolume*> _undoStates;
	uint8_t _undoIndex = 0u;
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
};

inline bool UndoHandler::canUndo() const {
	return _undoIndex > 0;
}

inline bool UndoHandler::canRedo() const {
	if (_undoStates.empty()) {
		return false;
	}
	return _undoIndex < _undoStates.size() - 1;
}

}
