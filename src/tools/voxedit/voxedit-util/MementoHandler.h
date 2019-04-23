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

enum class MementoType {
	Modification,
	LayerAdded,
	LayerDeleted
};

struct LayerState {
	MementoType type;
	voxel::RawVolume* volume;
	int layer;
	std::string name;
	voxel::Region region = voxel::Region::InvalidRegion;
};

// TODO: support partial volumes (dirty regions - see SceneManager - will reduce memory a lot)
class MementoHandler : public core::IComponent {
private:
	std::vector<LayerState> _states;
	uint8_t _statePosition = 0u;
	int _locked = 0;
public:
	static const int MaxStates;

	MementoHandler();
	~MementoHandler();

	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @brief Locks the handler for accepting new states or perform undo() or redo() steps
	 * @sa @c unlock()
	 */
	void lock();
	/**
	 * @brief Unlocks the handler for accepting new states or perform undo() or redo() steps again
	 * @sa @c lock()
	 */
	void unlock();

	void clearStates();
	/**
	 * @brief Add a new state entry to the memento handler that you can return to.
	 * @note This is adding the current active state to the handler - you can then undo to the previous state.
	 * That is the reason why you always have to add the initial (maybe empty) state, too
	 * @note Keep in mind, that there is a maximum of states that can get handled here.
	 * @param[in] layer The layer id that was modified
	 * @param[in] name The name of the layer
	 * @param[in] volume The state of the volume
	 * @param[in] type The @c MementoType - has influence on undo() and redo() state position changes.
	 */
	void markUndo(int layer, const std::string& name, const voxel::RawVolume* volume, MementoType type = MementoType::Modification, const voxel::Region& region = voxel::Region::InvalidRegion);
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
