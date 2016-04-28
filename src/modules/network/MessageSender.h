#pragma once

#include "Network.h"
#include "messages/ServerMessages_generated.h"
#include "messages/ClientMessages_generated.h"
#include <memory>

using namespace network::messages::server;
using namespace network::messages::client;

#define sendServerMsgMultiple(peers, numPeers, msg, type) _messageSender->sendServerMessage(peers, numPeers, fbb, network::messages::server::Type_##type, network::messages::server::Create##msg.Union());
#define sendServerMsg(msg, type) _messageSender->sendServerMessage(_peer, fbb, network::messages::server::Type_##type, network::messages::server::Create##msg.Union());
#define broadcastServerMsg(msg, type) _messageSender->broadcastServerMessage(fbb, network::messages::server::Type_##type, network::messages::server::Create##msg.Union());
#define sendClientMsg(msg, type) _messageSender->sendClientMessage(_peer, fbb, network::messages::client::Type_##type, network::messages::client::Create##msg.Union());

namespace network {

using namespace flatbuffers;

class MessageSender {
private:
	NetworkPtr _network;

public:
	MessageSender(NetworkPtr network);

	void sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	void broadcastServerMessage(FlatBufferBuilder& fbb, messages::server::Type type, Offset<void> data, int channel = 0, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	/**
	 * @brief Sends a message to the client
	 */
	void sendClientMessage(ENetPeer* peer, FlatBufferBuilder& fbb, messages::client::Type type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<MessageSender> MessageSenderPtr;

}
