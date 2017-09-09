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

enum class DisconnectReason {
	ProtocolError,
	Disconnect,
	Unknown
};

class Network {
protected:
	ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	core::EventBusPtr _eventBus;

	/**
	 * @brief Package deserialization
	 * @return @c false if the package couldn't get deserialized properly or no handler is registered for the found message
	 * @c true if everything went smooth.
	 */
	virtual bool packetReceived(ENetEvent& event) = 0;
	void disconnectPeer(ENetPeer *peer, DisconnectReason reason);
	void updateHost(ENetHost* host);
public:
	Network(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);
	virtual ~Network();

	virtual bool init();
	virtual void shutdown();

	const ProtocolHandlerRegistryPtr& registry();

	inline bool sendMessage(ENetPeer* peer, ENetPacket* packet, int channel = 0) {
		return enet_peer_send(peer, channel, packet) == 0;
	}
};

inline const ProtocolHandlerRegistryPtr& Network::registry() {
	return _protocolHandlerRegistry;
}

typedef std::shared_ptr<Network> NetworkPtr;

}
