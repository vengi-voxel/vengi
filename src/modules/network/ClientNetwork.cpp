/**
 * @file
 */

#include "ServerMessages_generated.h"
#include "ClientNetwork.h"
#include "core/Assert.h"
#include "core/Trace.h"
#include "core/Log.h"

namespace network {

ClientNetwork::ClientNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus) :
		Super(protocolHandlerRegistry, eventBus) {
}

void ClientNetwork::shutdown() {
	disconnect();
	Super::shutdown();
}

bool ClientNetwork::packetReceived(ENetEvent& event) {
	flatbuffers::Verifier v(event.packet->data, event.packet->dataLength);

	if (!VerifyServerMessageBuffer(v)) {
		Log::error("Illegal server packet received with length: %i", (int)event.packet->dataLength);
		return false;
	}
	const ServerMessage *req = GetServerMessage(event.packet->data);
	ServerMsgType type = req->data_type();
	ProtocolHandlerPtr handler = _protocolHandlerRegistry->getHandler(EnumNameServerMsgType(type));
	if (!handler) {
		Log::error("No handler for server msg type %s", EnumNameServerMsgType(type));
		return false;
	}
	Log::debug("Received %s", EnumNameServerMsgType(type));
	handler->execute(event.peer, reinterpret_cast<const flatbuffers::Table*>(req->data()));
	return true;
}

ENetPeer* ClientNetwork::connect(uint16_t port, const std::string& hostname, int maxChannels) {
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

void ClientNetwork::destroy() {
	enet_host_destroy(_client);
	_client = nullptr;
}

void ClientNetwork::disconnect() {
	if (_client == nullptr) {
		return;
	}

	enet_host_flush(_client);

	for (size_t i = 0; i < _client->peerCount; ++i) {
		ENetPeer *peer = &_client->peers[i];
		disconnectPeer(peer, DisconnectReason::Disconnect);
	}
}

bool ClientNetwork::isConnected() const {
	return _client != nullptr;
}

void ClientNetwork::update() {
	core_trace_scoped(Network);
	updateHost(_client);
}

}
