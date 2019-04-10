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

struct LayerState {
	voxel::RawVolume* volume;
	int layer;
	std::string name;
};

class MementoHandler {
private:
	std::vector<LayerState> _states;
	uint8_t _statePosition = 0u;
	static constexpr int _maxStates = 64;

public:
	MementoHandler();
	~MementoHandler();

	void clearStates();
	void markUndo(int layer, const std::string& name, const voxel::RawVolume* volume);

	LayerState undo();
	LayerState redo();
	bool canUndo() const;
	bool canRedo() const;

	const LayerState& state() const;

	size_t stateSize() const;
	uint8_t statePosition() const;
};

inline const LayerState& MementoHandler::state() const {
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
