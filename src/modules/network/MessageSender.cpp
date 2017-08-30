/**
 * @file
 */

#include "MessageSender.h"
#include "core/Log.h"
#include "core/Common.h"

namespace network {

inline ENetPacket* createServerPacket(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateServerMessage(fbb, type, data);
	FinishServerMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create server package: %s - size %ui", EnumNameServerMsgType(type), fbb.GetSize());
	return packet;
}

inline ENetPacket* createClientPacket(FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateClientMessage(fbb, type, data);
	FinishClientMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create client package: %s - size %ui", EnumNameClientMsgType(type), fbb.GetSize());
	return packet;
}

MessageSender::MessageSender(const NetworkPtr& network) :
		_network(network) {
}

void MessageSender::sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	core_assert(peer != nullptr);
	sendServerMessage(&peer, 1, fbb, type, data, flags);
}

void MessageSender::sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	Log::debug("Send %s", EnumNameServerMsgType(type));
	core_assert(numPeers > 0);
	auto packet = createServerPacket(fbb, type, data, flags);
	for (int i = 0; i < numPeers; ++i) {
		const bool success = _network->sendMessage(peers[i], packet);
		if (success) {
			continue;
		}
		Log::debug("Failed to send the message %s to peer %i (State: %i)", EnumNameServerMsgType(type), i, peers[i]->state);
	}
	fbb.Clear();
}

void MessageSender::broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel, uint32_t flags) {
	Log::debug("Broadcast %s", EnumNameServerMsgType(type));
	_network->broadcast(createServerPacket(fbb, type, data, flags), channel);
	fbb.Clear();
}

void MessageSender::sendClientMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags) {
	if (peer == nullptr) {
		Log::debug("don't send client message, no peer available");
		return;
	}
	_network->sendMessage(peer, createClientPacket(fbb, type, data, flags));
	fbb.Clear();
}

}
