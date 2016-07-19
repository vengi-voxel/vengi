/**
 * @file
 */

#include "MessageSender.h"
#include "core/Log.h"
#include "core/Common.h"

namespace network {

inline ENetPacket* createServerPacket(FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, uint32_t flags) {
	auto msg = messages::server::CreateServerMessage(fbb, type, data);
	messages::server::FinishServerMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create server package: %s - size %ui", messages::server::EnumNameType(type), fbb.GetSize());
	return packet;
}

inline ENetPacket* createClientPacket(FlatBufferBuilder& fbb, messages::client::Type type, Offset<void> data, uint32_t flags) {
	auto msg = messages::client::CreateClientMessage(fbb, type, data);
	messages::client::FinishClientMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create client package: %s - size %ui", messages::client::EnumNameType(type), fbb.GetSize());
	return packet;
}

MessageSender::MessageSender(NetworkPtr network) :
		_network(network) {
}

void MessageSender::sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, uint32_t flags) {
	core_assert(peer != nullptr);
	sendServerMessage(&peer, 1, fbb, type, data, flags);
}

void MessageSender::sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, uint32_t flags) {
	Log::debug("Send %s", messages::server::EnumNameType(type));
	core_assert(numPeers > 0);
	auto packet = createServerPacket(fbb, type, data, flags);
	for (int i = 0; i < numPeers; ++i) {
		const bool success = _network->sendMessage(peers[i], packet);
		if (success)
			continue;
		Log::error("Failed to send the message %s to peer %i (State: %i)", messages::server::EnumNameType(type), i, peers[i]->state);
	}
}

void MessageSender::broadcastServerMessage(FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, int channel, uint32_t flags) {
	Log::debug("Broadcast %s", messages::server::EnumNameType(type));
	_network->broadcast(createServerPacket(fbb, type, data, flags), channel);
}

void MessageSender::sendClientMessage(ENetPeer* peer, FlatBufferBuilder& fbb, messages::client::Type type, Offset<void> data, uint32_t flags) {
	if (peer == nullptr) {
		Log::debug("don't send client message, no peer available");
		return;
	}
	_network->sendMessage(peer, createClientPacket(fbb, type, data, flags));
}

}
