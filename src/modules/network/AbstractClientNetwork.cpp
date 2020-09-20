/**
 * @file
 */

#include "AbstractClientNetwork.h"
#include "core/Assert.h"
#include "core/Trace.h"
#include "core/Log.h"

namespace network {

AbstractClientNetwork::AbstractClientNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus) :
		Super(protocolHandlerRegistry, eventBus) {
}

void AbstractClientNetwork::shutdown() {
	disconnect();
	destroy();
	Super::shutdown();
}


ENetPeer* AbstractClientNetwork::connect(uint16_t port, const core::String& hostname, int maxChannels) {
	if (_client) {
		disconnect();
	}
	_client = enet_host_create(
			nullptr,
			1, /* only allow 1 outgoing connection */
			maxChannels,
			57600 / 8, /* 56K modem with 56 Kbps downstream bandwidth */
			14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */
			);
	if (_client == nullptr) {
		Log::error("Failed to create host");
		return nullptr;
	}
	enet_host_compress_with_range_coder(_client);

	ENetAddress address;
	enet_address_set_host(&address, hostname.c_str());
	address.port = port;

	_peer = enet_host_connect(_client, &address, maxChannels, 0);
	if (_peer == nullptr) {
		Log::error("Failed to connect to peer");
		return nullptr;
	}

	enet_host_flush(_client);
	enet_peer_timeout(_peer, ENET_PEER_TIMEOUT_LIMIT, ENET_PEER_TIMEOUT_MINIMUM, ENET_PEER_TIMEOUT_MAXIMUM);
	core_assert(_peer->state == ENET_PEER_STATE_CONNECTING);
	return _peer;
}

void AbstractClientNetwork::destroy() {
	enet_host_destroy(_client);
	_client = nullptr;
	_peer = nullptr;
}

void AbstractClientNetwork::disconnect() {
	if (_client == nullptr) {
		return;
	}

	enet_host_flush(_client);

	const size_t peerCount = _client->peerCount;
	for (size_t i = 0; i < peerCount; ++i) {
		ENetPeer *peer = &_client->peers[i];
		disconnectPeer(peer, DisconnectReason::Disconnect);
	}
}

bool AbstractClientNetwork::isConnecting() const {
	if (_client == nullptr) {
		return false;
	}
	if (_client->peerCount == 0u) {
		return false;
	}
	const ENetPeer& peer = _client->peers[0];
	return peer.state == ENET_PEER_STATE_CONNECTING || peer.state == ENET_PEER_STATE_CONNECTION_PENDING ||
			peer.state == ENET_PEER_STATE_ACKNOWLEDGING_CONNECT || peer.state == ENET_PEER_STATE_CONNECTION_SUCCEEDED;
}

bool AbstractClientNetwork::isConnected() const {
	if (_client == nullptr) {
		return false;
	}
	if (_client->peerCount == 0u) {
		return false;
	}
	const ENetPeer& peer = _client->peers[0];
	return peer.state == ENET_PEER_STATE_CONNECTED;
}

bool AbstractClientNetwork::isDisconnected() const {
	if (_client == nullptr) {
		return true;
	}
	if (_client->peerCount == 0u) {
		return true;
	}
	const ENetPeer& peer = _client->peers[0];
	return peer.state == ENET_PEER_STATE_DISCONNECTED;
}

bool AbstractClientNetwork::isDisconnecting() const {
	if (_client == nullptr) {
		return false;
	}
	if (_client->peerCount == 0u) {
		return false;
	}
	const ENetPeer& peer = _client->peers[0];
	return peer.state == ENET_PEER_STATE_DISCONNECT_LATER || peer.state == ENET_PEER_STATE_DISCONNECTING ||
			peer.state == ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT;
}

void AbstractClientNetwork::update() {
	core_trace_scoped(Network);
	updateHost(_client);
}

}
