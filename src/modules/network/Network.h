/**
 * @file
 */

#pragma once

#include "ProtocolHandlerRegistry.h"
#include "IMsgProtocolHandler.h"
#include "core/EventBus.h"
#include <string>
#include <stdint.h>
#include <list>
#include <memory>

namespace network {

class Network {
private:
	ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	core::EventBusPtr _eventBus;
	ENetHost* _server;
	ENetHost* _client;

	bool packetReceived(ENetEvent& event, bool server);
	void disconnectPeer(ENetPeer *peer, uint32_t timeout = 3000);
	void updateHost(ENetHost* host, bool server);
public:
	Network(ProtocolHandlerRegistryPtr protocolHandlerRegistry, core::EventBusPtr eventBus);
	virtual ~Network();

	bool start();
	void update();

	// Server related methods
	bool bind(uint16_t port, const std::string& hostname = "", int maxPeers = 1024, int maxChannels = 1);
	inline void broadcast(ENetPacket* packet, int channel = 0) { enet_host_broadcast(_server, channel, packet); }

	// Client related methods
	ENetPeer* connect(uint16_t port, const std::string& hostname, int maxChannels = 1);
	void disconnect();

	// Shared methods
	inline bool sendMessage(ENetPeer* peer, ENetPacket* packet, int channel = 0) { return enet_peer_send(peer, channel, packet) == 0; }
};

typedef std::shared_ptr<Network> NetworkPtr;

}
