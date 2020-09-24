/**
 * @file
 */

#include "MessageSender.h"
#include "core/Log.h"
#include "core/Common.h"

namespace network {

inline ENetPacket* createPacket(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateMessage(fbb, type, data);
	FinishMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create package: %s - size %u", EnumNameMsgType(type), fbb.GetSize());
	return packet;
}

MessageSender::MessageSender(const AINetworkPtr& network) :
		_network(network) {
}

bool MessageSender::sendMessage(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags) {
	const bool retVal = _network->sendMessage(createPacket(fbb, type, data, flags));
	fbb.Clear();
	return retVal;
}

}
