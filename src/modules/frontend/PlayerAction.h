/**
 * @file
 */

#pragma once

#include "command/ActionButton.h"
#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "Shared_generated.h"

namespace frontend {

class ClientEntity;
typedef core::SharedPtr<ClientEntity> ClientEntityPtr;

/**
 * @brief Trigger action component that does the input listening
 *
 * @see core::ActionButton
 */
class PlayerAction : public core::IComponent {
private:
	core::ActionButton _triggerAction;
	int _triggerActionCounter = 0;
public:
	bool init() override;
	void update(double nowSeconds, const ClientEntityPtr& entity);
	void construct() override;
	void shutdown() override;
	/**
	 * @return @c true if there are still queued actions left
	 */
	bool popTriggerAction();
};

inline bool PlayerAction::popTriggerAction() {
	if (_triggerActionCounter <= 0) {
		return false;
	}
	return --_triggerActionCounter >= 0;
}

}
