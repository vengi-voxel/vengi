/**
 * @file
 */

#pragma once

#include "Network.h"

namespace network {

class ServerNetwork : public Network {
private:
	ENetHost* _server = nullptr;
	using Super = Network;
public:
	ServerNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);

	bool bind(uint16_t port, const std::string& hostname = "", int maxPeers = 1024, int maxChannels = 1);
	bool packetReceived(ENetEvent& event) override;

	bool broadcast(ENetPacket* packet, int channel = 0);

	void update();
	void shutdown() override;
};

typedef std::shared_ptr<ServerNetwork> ServerNetworkPtr;

}
