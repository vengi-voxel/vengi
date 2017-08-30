/**
 * @file
 */

#pragma once

#include "Network.h"
#include "ServerMessages_generated.h"
#include "ClientMessages_generated.h"
#include <memory>

namespace network {

using namespace flatbuffers;

class MessageSender {
private:
	NetworkPtr _network;

public:
	MessageSender(const NetworkPtr& network);

	void sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel = 0, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	/**
	 * @brief Sends a message to the client
	 */
	void sendClientMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<MessageSender> MessageSenderPtr;

inline void MessageSender::sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	sendServerMessage(&peers.front(), peers.size(), fbb, type, data, flags);
}


}
