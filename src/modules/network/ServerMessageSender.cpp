/**
 * @file
 */

#include "ServerMessageSender.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Assert.h"

namespace network {

inline ENetPacket* createServerPacket(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateServerMessage(fbb, type, data);
	FinishServerMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create server package: %s - size %ui", EnumNameServerMsgType(type), fbb.GetSize());
	return packet;
}

ServerMessageSender::ServerMessageSender(const ServerNetworkPtr& network) :
		_network(network) {
}

void ServerMessageSender::sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	core_assert(peer != nullptr);
	sendServerMessage(&peer, 1, fbb, type, data, flags);
}

void ServerMessageSender::sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
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

void ServerMessageSender::broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel, uint32_t flags) {
	Log::debug("Broadcast %s", EnumNameServerMsgType(type));
	_network->broadcast(createServerPacket(fbb, type, data, flags), channel);
	fbb.Clear();
}

}
