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

	inline void broadcast(ENetPacket* packet, int channel = 0) {
		enet_host_broadcast(_server, channel, packet);
	}

	void update();
	void shutdown();
};

typedef std::shared_ptr<ServerNetwork> ServerNetworkPtr;

}
