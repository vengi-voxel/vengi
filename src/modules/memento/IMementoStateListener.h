/**
 * @file
 */

#pragma once

namespace memento {

// Forward declaration
struct MementoState;

/**
 * @brief Interface for listening to memento state changes
 * Implementations can be notified when new memento states are created
 */
class IMementoStateListener {
public:
	virtual ~IMementoStateListener() = default;

	/**
	 * @brief Called when a new memento state is added
	 * @param state The newly added memento state
	 */
	virtual void onMementoStateAdded(const MementoState &state) = 0;
	/**
	 * @brief Called when a memento state is skipped (not added) due to locked state (might happen during undo/redo
	 * operations)
	 * @param state The memento state that was skipped
	 */
	virtual void onMementoStateSkipped(const MementoState &state) = 0;
};

} // namespace memento
