/**
 * @file
 */

#include "Network.h"
#include "IProtocolHandler.h"
#include "ProtocolHandlerRegistry.h"
#include "NetworkEvents.h"
#include "core/Trace.h"
#include "core/Enum.h"
#include "core/Log.h"

#include <memory>

namespace network {

Network::Network(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus) :
		_protocolHandlerRegistry(protocolHandlerRegistry), _eventBus(eventBus) {
}

Network::~Network() {
	shutdown();
}

void Network::shutdown() {
	enet_deinitialize();
	_protocolHandlerRegistry->shutdown();
}

bool Network::init() {
	if (enet_initialize() != 0) {
		return false;
	}
	enet_time_set(0);
	return true;
}

bool Network::disconnectPeer(ENetPeer *peer, DisconnectReason reason) {
	if (peer == nullptr) {
		return false;
	}
	Log::info("trying to disconnect peer: %u", peer->connectID);
	enet_peer_disconnect(peer, core::enumVal(reason));
	if (peer->state == ENET_PEER_STATE_DISCONNECTED) {
		_eventBus->publish(DisconnectEvent(peer, reason));
	}
	return true;
}

void Network::updateHost(ENetHost* host) {
	if (host == nullptr) {
		return;
	}
	enet_host_flush(host);
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		core_trace_scoped(NetworkEventHandling);
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT: {
			core_trace_scoped(NetworkConnect);
			Log::info("New connection event received");
			_eventBus->publish(NewConnectionEvent(event.peer));
			break;
		}
		case ENET_EVENT_TYPE_RECEIVE: {
			core_trace_scoped(NetworkPacket);
			Log::trace("Package received");
			if (!packetReceived(event)) {
				Log::error("Failure while receiving a package - disconnecting now...");
				disconnectPeer(event.peer, DisconnectReason::ProtocolError);
			}
			enet_packet_destroy(event.packet);
			break;
		}
		case ENET_EVENT_TYPE_DISCONNECT: {
			core_trace_scoped(NetworkDisconnect);
			Log::info("New disconnect event received");
			_eventBus->publish(DisconnectEvent(event.peer, (DisconnectReason)event.data));
			break;
		}
		case ENET_EVENT_TYPE_NONE: {
			break;
		}
		}
	}
}

}
