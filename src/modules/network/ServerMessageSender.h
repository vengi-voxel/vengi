/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "ServerNetwork.h"
#include <memory>

namespace network {

using namespace flatbuffers;

/**
 * @brief Send messages from the server to the client(s)
 */
class ServerMessageSender {
private:
	ServerNetworkPtr _network;

public:
	ServerMessageSender(const ServerNetworkPtr& network);

	void sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel = 0, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<ServerMessageSender> ServerMessageSenderPtr;

inline void ServerMessageSender::sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	sendServerMessage(&peers.front(), peers.size(), fbb, type, data, flags);
}

}
