/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
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

class MementoHandler : public core::IComponent {
private:
	std::vector<LayerState> _states;
	uint8_t _statePosition = 0u;
	int _locked = 0;
public:
	static const int MaxStates;

	MementoHandler();
	~MementoHandler();

	void construct();
	bool init();
	void shutdown();

	void lock();
	void unlock();

	void clearStates();
	void markUndo(int layer, const std::string& name, const voxel::RawVolume* volume);
	void markLayerDeleted(int layer, const std::string& name, const voxel::RawVolume* volume);
	void markLayerAdded(int layer, const std::string& name, const voxel::RawVolume* volume);

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
	if (_locked > 0) {
		return false;
	}
	if (stateSize() <= 1) {
		return false;
	}
	return _statePosition > 0;
}

inline bool MementoHandler::canRedo() const {
	if (_locked > 0) {
		return false;
	}
	if (_states.empty()) {
		return false;
	}
	return _statePosition < stateSize() - 1;
}

}
