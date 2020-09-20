/**
 * @file
 */

#pragma once

#include "network/Network.h"

namespace network {

class AbstractClientNetwork : public Network {
private:
	ENetHost* _client = nullptr;
	ENetPeer* _peer = nullptr;
	using Super = Network;
public:
	AbstractClientNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);

	ENetPeer* connect(uint16_t port, const core::String& hostname, int maxChannels = 1);
	void disconnect();

	bool isConnecting() const;
	bool isConnected() const;
	bool isDisconnected() const;
	bool isDisconnecting() const;

	inline bool sendMessage(ENetPacket* packet, int channel = 0) {
		if (_peer == nullptr) {
			enet_packet_destroy(packet);
			return false;
		}
		if (packet == nullptr) {
			return false;
		}
		return Super::sendMessage(_peer, packet, channel);
	}

	void destroy();

	void update();
	void shutdown() override;
};

}
