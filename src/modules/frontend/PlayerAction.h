/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
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
public:
	bool init() override;
	void update(const ClientEntityPtr& entity);
	void construct() override;
	void shutdown() override;
};

}
