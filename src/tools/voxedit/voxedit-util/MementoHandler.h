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

// TODO: move to layers, not just volumes
class MementoHandler {
private:
	std::vector<voxel::RawVolume*> _states;
	std::vector<int> _layers;
	uint8_t _statePosition = 0u;
	static constexpr int _maxStates = 64;

public:
	MementoHandler();
	~MementoHandler();

	void clearStates();
	void markUndo(int layer, const voxel::RawVolume* volume);

	std::pair<int, voxel::RawVolume*> undo();
	std::pair<int, voxel::RawVolume*> redo();
	bool canUndo() const;
	bool canRedo() const;

	std::pair<int, voxel::RawVolume*> state() const;

	size_t stateSize() const;
	uint8_t statePosition() const;
};

inline std::pair<int, voxel::RawVolume*> MementoHandler::state() const {
	return std::make_pair(_layers[_statePosition], _states[_statePosition]);
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
