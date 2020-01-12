/**
 * @file
 * @defgroup Network
 * @{
 *
 * The network messages are defined in fbs files in the definition/ folder.
 */

#pragma once

#include "ProtocolHandlerRegistry.h"
#include "IMsgProtocolHandler.h"
#include "core/EventBus.h"
#include "core/IComponent.h"
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

/**
 * @brief Network implementation based on enet and flatbuffers
 */
class Network : public core::IComponent {
protected:
	ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	core::EventBusPtr _eventBus;

	/**
	 * @brief Package deserialization
	 * @return @c false if the package couldn't get deserialized properly or no handler is registered for the found message
	 * @c true if everything went smooth.
	 */
	virtual bool packetReceived(ENetEvent& event) = 0;
	bool disconnectPeer(ENetPeer *peer, DisconnectReason reason);
	void updateHost(ENetHost* host);
public:
	Network(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus);
	virtual ~Network();

	virtual bool init() override;
	virtual void shutdown() override;

	const ProtocolHandlerRegistryPtr& registry();

	bool sendMessage(ENetPeer* peer, ENetPacket* packet, int channel = 0);
};

inline bool Network::sendMessage(ENetPeer* peer, ENetPacket* packet, int channel) {
	if (packet->dataLength >= peer->host->maximumPacketSize) {
		Log::error("Packet is too big: %i - max allowed is %i", (int)packet->dataLength, (int)peer->host->maximumPacketSize);
		return false;
	}
	if (enet_peer_send(peer, channel, packet) == 0) {
		return true;
	}
	enet_packet_destroy(packet);
	return false;
}

inline const ProtocolHandlerRegistryPtr& Network::registry() {
	return _protocolHandlerRegistry;
}

typedef std::shared_ptr<Network> NetworkPtr;

}

/**
 * @}
 */
