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
	Log::trace("Create server package: %s - size %u", EnumNameServerMsgType(type), fbb.GetSize());
	return packet;
}

ServerMessageSender::ServerMessageSender(const ServerNetworkPtr& network) :
		_network(network) {
}

bool ServerMessageSender::sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	core_assert(peer != nullptr);
	return sendServerMessage(&peer, 1, fbb, type, data, flags);
}

bool ServerMessageSender::sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	Log::debug(logid, "Send %s", EnumNameServerMsgType(type));
	core_assert(numPeers > 0);
	int sent = 0;
	auto packet = createServerPacket(fbb, type, data, flags);
	{
		// TODO: lock
		for (int i = 0; i < numPeers; ++i) {
			if (!_network->sendMessage(peers[i], packet)) {
				Log::warn(logid, "Could not send message of type %s to peer %i", network::EnumNameServerMsgType(type), i);
			} else {
				++sent;
			}
		}
	}
	fbb.Clear();
	return sent == numPeers;
}

bool ServerMessageSender::broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel, uint32_t flags) {
	Log::debug(logid, "Broadcast %s on channel %i", EnumNameServerMsgType(type), channel);
	bool success = false;
	{
		// TODO: lock
		success = _network->broadcast(createServerPacket(fbb, type, data, flags), channel);
	}
	fbb.Clear();
	return success;
}

}
