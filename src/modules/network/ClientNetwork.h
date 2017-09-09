#pragma once

#include "Network.h"

namespace network {

class ClientNetwork : public Network {
private:
	ENetHost* _client = nullptr;
	ENetPeer* _peer = nullptr;
	using Super = Network;
public:
	ClientNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);

	ENetPeer* connect(uint16_t port, const std::string& hostname, int maxChannels = 1);
	void disconnect();
	bool packetReceived(ENetEvent& event) override;

	inline bool sendMessage(ENetPacket* packet, int channel = 0) {
		return Super::sendMessage(_peer, packet, channel);
	}

	void update();
	void shutdown();
};

typedef std::shared_ptr<ClientNetwork> ClientNetworkPtr;

}
