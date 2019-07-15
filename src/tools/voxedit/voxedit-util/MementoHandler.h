/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/Voxel.h"
#include <vector>
#include <string>
#include <stdint.h>
#include <stddef.h>

namespace voxel {
class RawVolume;
}

namespace voxedit {

enum class MementoType {
	Modification,
	LayerAdded,
	LayerDeleted,
	LayerRenamed
};

struct LayerVolumeData {
	size_t compressedBufferSize;
	uint8_t* data;
	voxel::Region region;

	static voxel::RawVolume* toVolume(const LayerVolumeData* data);
	static LayerVolumeData* fromVolume(const voxel::RawVolume* volume);
};

struct LayerState {
	MementoType type;
	LayerVolumeData* volume;
	int layer;
	std::string name;
	voxel::Region region;
};

// TODO: support partial volumes (dirty regions - see SceneManager - will reduce memory a lot)
// TODO: rle or zlib compression for the stored data to reduce memory
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

	/**
	 * @note Keep in mind that the returned state contains memory for the voxel::RawVolume that you take ownership for
	 */
	LayerState undo();
	/**
	 * @note Keep in mind that the returned state contains memory for the voxel::RawVolume that you take ownership for
	 */
	LayerState redo();
	bool canUndo() const;
	bool canRedo() const;

	const LayerState& state() const;

	size_t stateSize() const;
	uint8_t statePosition() const;
};

/**
 * @brief Locks the memento handler to accept further state changes for undo/redo.
 * @note This is useful in situations where an undo or redo would result in actions that by
 * itself would generate new memento states, too.
 */
class ScopedMementoHandlerLock {
private:
	MementoHandler& _handler;
public:
	ScopedMementoHandlerLock(MementoHandler& handler) : _handler(handler) {
		_handler.lock();
	}
	~ScopedMementoHandlerLock() {
		_handler.unlock();
	}
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
