/**
 * @file
 */

#pragma once

#include "command/ActionButton.h"
#include "ModifierType.h"

namespace voxedit {

class SceneManager;

/**
 * @brief This action button executes the current selected Modifier action.
 *
 * @sa ModifierType
 * @sa Modifier
 */
class ModifierButton : public command::ActionButton {
private:
	using Super = command::ActionButton;
	SceneManager *_sceneMgr;
	ModifierType _newType;
	bool _sliceAction = false;
	bool _ownsPress = false;
	uint64_t _editId = 0;
public:
	/**
	 * @param[in] newType This ModifierType is set if the action button is triggered. No matter which type is
	 * set currently. The old value is restored if the action button is released.
	 */
	ModifierButton(SceneManager *sceneMgr, ModifierType newType = ModifierType::None);

	/**
	 * @brief Execute the @c ModifierType action
	 *
	 * @param continuous @c true for mid-stroke dabs (keep the brush open). @c false ends the brush
	 *        after this execution (mouse up / final dab).
	 *
	 * @sa Modifier::beginEdit()
	 * @sa Modifier::updateEdit()
	 * @sa Modifier::finishEdit()
	 * @sa Modifier::abort()
	 */
	void execute(bool continuous);
	bool slicePressed() const { return _sliceAction && pressed(); }
	bool ownsPress() const { return _ownsPress && pressed(); }
	bool handleDown(int32_t key, double pressedMillis) override;
	bool handleUp(int32_t key, double releasedMillis) override;
};

}
