/**
 * @file
 */

#include "ServerMessageSender.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/StringUtil.h"

namespace network {

ENetPacket* ServerMessageSender::createServerPacket(ServerMsgType type, const void * data, size_t dataLength, uint32_t flags) {
	ENetPacket* packet = enet_packet_create(data, dataLength, flags);
	const char *msgType = EnumNameServerMsgType(type);
	Log::trace(logid, "Create server package: %s - size %u", msgType, (unsigned int)dataLength);
	const metric::TagMap& tags {{"direction", "out"}, {"type", msgType}};
	_metric->count("network_packet_count", 1, tags);
	_metric->count("network_packet_size", (int)dataLength, tags);
	return packet;
}

ENetPacket* ServerMessageSender::createServerPacket(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateServerMessage(fbb, type, data);
	FinishServerMessageBuffer(fbb, msg);
	return createServerPacket(type, fbb.GetBufferPointer(), fbb.GetSize(), flags);
}

ServerMessageSender::ServerMessageSender(const ServerNetworkPtr& network, const metric::MetricPtr& metric) :
		_network(network), _metric(metric) {
}

bool ServerMessageSender::sendServerMessage(ENetPeer* peer, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	core_assert(peer != nullptr);
	return sendServerMessage(&peer, 1, fbb, type, data, flags);
}

bool ServerMessageSender::sendServerMessage(ENetPeer** peers, int numPeers, FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, uint32_t flags) {
	const char *msgType = network::EnumNameServerMsgType(type);
	Log::debug(logid, "Send %s to %i peers", msgType, numPeers);
	core_assert(numPeers > 0);
	int sent = 0;
	int notsent = 0;
	auto packet = createServerPacket(fbb, type, data, flags);
	const metric::TagMap& tags {{"direction", "out"}, {"type", msgType}};
	{
		// TODO: lock
		for (int i = 0; i < numPeers; ++i) {
			if (!_network->sendMessage(peers[i], packet)) {
				++notsent;
				Log::trace(logid, "Could not send message of type %s to peer %i", msgType, i);
			} else {
				++sent;
			}
		}
		if (notsent > 0) {
			_metric->count("network_not_sent", notsent, tags);
		}
		if (sent > 0) {
			_metric->count("network_sent", sent, tags);
		}
	}
	fbb.Clear();
	return sent == numPeers;
}

bool ServerMessageSender::broadcastServerMessage(FlatBufferBuilder& fbb, ServerMsgType type, Offset<void> data, int channel, uint32_t flags) {
	const char *msgType = network::EnumNameServerMsgType(type);
	Log::debug(logid, "Broadcast %s on channel %i", msgType, channel);
	bool success = false;
	{
		// TODO: lock
		success = _network->broadcast(createServerPacket(fbb, type, data, flags), channel);
		const metric::TagMap& tags {{"direction", "broadcast"}, {"type", msgType}};
		_metric->count("network_sent", 1, tags);
	}
	fbb.Clear();
	return success;
}

}
