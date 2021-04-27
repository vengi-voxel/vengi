/**
 * @file
 */

#include "AIMessageSender.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/StringUtil.h"

namespace network {

ENetPacket* AIMessageSender::createServerPacket(ai::MsgType type, const void * data, size_t dataLength, uint32_t flags) {
	ENetPacket* packet = enet_packet_create(data, dataLength, flags);
	const char *msgType = EnumNameMsgType(type);
	Log::trace(logid, "Create server package: %s - size %u", msgType, (unsigned int)dataLength);
	const metric::TagMap& tags {{"direction", "out"}, {"type", msgType}};
	_metric->count("network_packet_count", 1, tags);
	_metric->count("network_packet_size", (int)dataLength, tags);
	return packet;
}

ENetPacket* AIMessageSender::createServerPacket(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateAIRootMessage(fbb, type, data);
	FinishAIRootMessageBuffer(fbb, msg);
	return createServerPacket(type, fbb.GetBufferPointer(), fbb.GetSize(), flags);
}

AIMessageSender::AIMessageSender(const AIServerNetworkPtr& network, const metric::MetricPtr& metric) :
		_network(network), _metric(metric) {
}

bool AIMessageSender::sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags) {
	core_assert(peer != nullptr);
	return sendServerMessage(&peer, 1, fbb, type, data, flags);
}

bool AIMessageSender::sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags) {
	const char *msgType = ai::EnumNameMsgType(type);
	Log::debug(logid, "Send %s to %i peers", msgType, numPeers);
	core_assert(numPeers > 0);
	int sent = 0;
	auto packet = createServerPacket(fbb, type, data, flags);
	const metric::TagMap& tags {{"direction", "out"}, {"type", msgType}};
	{
		for (int i = 0; i < numPeers; ++i) {
			if (!_network->sendMessage(peers[i], packet)) {
				_metric->count("network_not_sent", 1, tags);
				Log::trace(logid, "Could not send message of type %s to peer %i", msgType, i);
			} else {
				_metric->count("network_sent", 1, tags);
				++sent;
			}
		}
	}
	fbb.Clear();
	return sent == numPeers;
}

bool AIMessageSender::broadcastServerMessage(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, int channel, uint32_t flags) {
	const char *msgType = ai::EnumNameMsgType(type);
	Log::debug(logid, "Broadcast %s on channel %i", msgType, channel);
	bool success = false;
	{
		success = _network->broadcast(createServerPacket(fbb, type, data, flags), channel);
		const metric::TagMap& tags {{"direction", "broadcast"}, {"type", msgType}};
		_metric->count("network_sent", 1, tags);
	}
	fbb.Clear();
	return success;
}

}
