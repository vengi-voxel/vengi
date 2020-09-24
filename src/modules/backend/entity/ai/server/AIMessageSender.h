/**
 * @file
 */

#pragma once

#include "AIMessages_generated.h"
#include "AIServerNetwork.h"
#include "metric/Metric.h"
#include "core/Log.h"
#include <memory>

namespace network {

using namespace flatbuffers;

/**
 * @brief Send messages from the server to the client(s)
 */
class AIMessageSender {
private:
	static constexpr auto logid = Log::logid("AIMessageSender");
	AIServerNetworkPtr _network;
	metric::MetricPtr _metric;

public:
	ENetPacket* createServerPacket(ai::MsgType type, const void * data, size_t dataLength, uint32_t flags);
	ENetPacket* createServerPacket(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags);
	AIMessageSender(const AIServerNetworkPtr& network, const metric::MetricPtr& metric);

	bool sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	bool sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	bool sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
	bool broadcastServerMessage(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, int channel = 0, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<AIMessageSender> AIMessageSenderPtr;

inline bool AIMessageSender::sendServerMessage(std::vector<ENetPeer*> peers, FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags) {
	return sendServerMessage(&peers.front(), peers.size(), fbb, type, data, flags);
}

}
