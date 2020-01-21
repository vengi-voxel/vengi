/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "ServerNetwork.h"
#include "core/metric/Metric.h"
#include "core/Log.h"
#include <memory>

namespace network {

using namespace flatbuffers;

/**
 * @brief Send messages from the server to the client(s)
 */
class ServerMessageSender {
private:
	static constexpr auto logid = Log::logid("ServerMessageSender");
	ServerNetworkPtr _network;
	metric::MetricPtr _metric;

	ENetPacket* createServerPacket(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags);

public:
	ServerMessageSender(const ServerNetworkPtr& network, const metric::MetricPtr& metric);

	bool sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	bool sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	bool sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	bool broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel = 0, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<ServerMessageSender> ServerMessageSenderPtr;

inline bool ServerMessageSender::sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	return sendServerMessage(&peers.front(), peers.size(), fbb, type, data, flags);
}

}
