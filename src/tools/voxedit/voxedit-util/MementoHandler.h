/**
 * @file
 */

#pragma once

#include <vector>
#include <stdint.h>
#include <stddef.h>

namespace voxel {
class RawVolume;
}

namespace voxedit {

class MementoHandler {
private:
	std::vector<voxel::RawVolume*> _states;
	uint8_t _statePosition = 0u;
	static constexpr int _maxStates = 64;

public:
	MementoHandler();
	~MementoHandler();

	void clearStates();
	void markUndo(const voxel::RawVolume* volume);

	voxel::RawVolume* undo();
	voxel::RawVolume* redo();
	bool canUndo() const;
	bool canRedo() const;

	voxel::RawVolume* state() const;

	size_t stateSize() const;
	uint8_t statePosition() const;
};

inline voxel::RawVolume* MementoHandler::state() const {
	return _states[_statePosition];
}

inline uint8_t MementoHandler::statePosition() const {
	return _statePosition;
}

inline size_t MementoHandler::stateSize() const {
	return _states.size();
}

inline bool MementoHandler::canUndo() const {
	if (stateSize() <= 1) {
		return false;
	}
	return _statePosition > 0;
}

inline bool MementoHandler::canRedo() const {
	if (_states.empty()) {
		return false;
	}
	return _statePosition < stateSize() - 1;
}

}
