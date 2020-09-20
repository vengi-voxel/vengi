/**
 * @file
 */

#pragma once

#include "network/AbstractClientNetwork.h"

namespace network {

class ClientNetwork : public AbstractClientNetwork {
private:
	using Super = AbstractClientNetwork;
public:
	ClientNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);

	bool packetReceived(ENetEvent& event) override;
};

typedef std::shared_ptr<ClientNetwork> ClientNetworkPtr;

}
