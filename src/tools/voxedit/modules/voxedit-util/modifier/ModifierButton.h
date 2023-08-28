/**
 * @file
 */

#pragma once

#include "command/ActionButton.h"
#include "ModifierType.h"

namespace voxedit {

/**
 * @brief This action button executes the current selected Modifier action.
 *
 * @sa ModifierType
 * @sa Modifier
 */
class ModifierButton : public command::ActionButton {
private:
	using Super = command::ActionButton;
	ModifierType _newType;
	ModifierType _oldType = ModifierType::None;
	// some actions might need a second action to complete the command
	bool _furtherAction = false;
public:
	/**
	 * @param[in] newType This ModifierType is set if the action button is triggered. No matter which type is
	 * set currently. The old value is restored if the action button is released.
	 */
	ModifierButton(ModifierType newType = ModifierType::None);

	/**
	 * @brief Execute the @c ModifierType action
	 *
	 * @param single @c false if the action should not abort the modifier execution @c true means that the next
	 * execution of the modifier action needs another @c handleDown() call.
	 *
	 * @sa Modifier::aabbStart()
	 * @sa Modifier::aabbAction()
	 * @sa Modifier::aabbAbort()
	 * @sa Modifier::aabbStep()
	 */
	void execute(bool single);
	bool handleDown(int32_t key, double pressedMillis) override;
	bool handleUp(int32_t key, double releasedMillis) override;
};

}
