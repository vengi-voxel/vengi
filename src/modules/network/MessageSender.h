/**
 * @file
 */

#pragma once

#include "Network.h"
#include "ServerMessages_generated.h"
#include "ClientMessages_generated.h"
#include <memory>

#define sendServerMsgMultiple(peers, numPeers, msg, type) _messageSender->sendServerMessage(peers, numPeers, fbb, network::ServerMsgType_##type, network::Create##msg.Union());
#define sendServerMsg(msg, type) _messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::type, network::Create##msg.Union());
#define broadcastServerMsg(msg, type) _messageSender->broadcastServerMessage(fbb, network::ServerMsgType::type, network::Create##msg.Union());
#define sendClientMsg(msg, type) _messageSender->sendClientMessage(_peer, fbb, network::Type::type, network::Create##msg.Union());

namespace network {

using namespace flatbuffers;

class MessageSender {
private:
	NetworkPtr _network;

public:
	MessageSender(NetworkPtr network);

	void sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel = 0, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	/**
	 * @brief Sends a message to the client
	 */
	void sendClientMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<MessageSender> MessageSenderPtr;

}
