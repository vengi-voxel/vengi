/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
#include "ModifierType.h"

namespace voxedit {

/**
 * @brief This action button executes the current selected Modifier action.
 *
 * @sa ModifierType
 * @sa Modifier
 */
class ModifierButton : public core::ActionButton {
private:
	using Super = core::ActionButton;
	ModifierType _newType;
	ModifierType _oldType = ModifierType::None;
	// some actions might need a second action to complete the command
	bool _secondAction = false;
public:
	/**
	 * @param[in] newType This ModifierType is set if the action button is triggered. No matter which type is
	 * set currently. The old value is restored if the action button is released.
	 */
	ModifierButton(ModifierType newType = ModifierType::None);

	void execute();
	bool handleDown(int32_t key, double pressedMillis) override;
	bool handleUp(int32_t key, double releasedMillis) override;
};

}
