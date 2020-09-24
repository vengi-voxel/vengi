/**
 * @file
 */

#pragma once

#include "network/AbstractClientNetwork.h"
#include "core/SharedPtr.h"

namespace network {

class AINetwork : public AbstractClientNetwork {
private:
	using Super = AbstractClientNetwork;
public:
	AINetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);

	bool packetReceived(ENetEvent& event) override;
};

typedef core::SharedPtr<AINetwork> AINetworkPtr;

}
